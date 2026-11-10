using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using ZwebtiMix;

class ReleaseStoreTests
{
    static int passed;
    static string fixture;
    static readonly string[] Runtime = { "launch.ps1", "runtime/rekhcom64.exe", "runtime/guest-us.bin", "runtime/SDL3.dll" };

    static void Check(bool value, string message)
    {
        if (!value) throw new Exception(message);
        passed++;
        Console.WriteLine("PASS " + message);
    }

    static string Archive(string name, string additional, string skip, bool symlink)
    {
        string path = Path.Combine(fixture, name + ".zip");
        using (var file = File.Create(path))
        using (var archive = new ZipArchive(file, ZipArchiveMode.Create))
        {
            foreach (string required in Runtime)
            {
                if (required == skip) continue;
                var entry = archive.CreateEntry(required);
                using (var writer = new StreamWriter(entry.Open(), Encoding.ASCII)) writer.Write(name + ": " + required);
            }
            if (additional != null)
            {
                var entry = archive.CreateEntry(additional);
                if (symlink) entry.ExternalAttributes = unchecked((int)(0xa1ff << 16));
                using (var writer = new StreamWriter(entry.Open(), Encoding.ASCII)) writer.Write("test fixture");
            }
        }
        return path;
    }

    static void Rejected(ReleaseStore store, string path, string version, string sha, string label)
    {
        string active = store.ActiveVersion;
        string runtime = File.ReadAllText(Path.Combine(store.ActiveDirectory, "runtime", "guest-us.bin"));
        bool rejected = false;
        try { store.Install(path, version, sha); }
        catch (InvalidDataException) { rejected = true; }
        Check(rejected, label + " rejected");
        Check(store.ActiveVersion == active && File.ReadAllText(Path.Combine(store.ActiveDirectory, "runtime", "guest-us.bin")) == runtime,
            label + " leaves current game unchanged");
    }

    static int Main()
    {
        fixture = Path.Combine(Path.GetTempPath(), "Zwebti-release-tests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(fixture);
        try
        {
            string root = Path.Combine(fixture, "install with spaces");
            var store = new ReleaseStore(root);
            Check(store.ActiveVersion == null && store.ActiveDirectory == null, "new installation has no active version");
            string first = Archive("first", null, null, false);
            store.Install(first, "0.6.0", ReleaseStore.Sha256(first));
            Check(store.ActiveVersion == "0.6.0" && File.Exists(Path.Combine(store.ActiveDirectory, "launch.ps1")), "verified complete update activates");
            string firstDirectory = store.ActiveDirectory;
            string data = Path.Combine(root, "profile", "data");
            Directory.CreateDirectory(data);
            File.WriteAllText(Path.Combine(data, "player.sav"), "SAVE MUST SURVIVE");
            string second = Archive("second", null, null, false);
            store.Install(second, "0.6.1", ReleaseStore.Sha256(second));
            Check(store.ActiveVersion == "0.6.1", "second version activates");
            Check(File.ReadAllText(Path.Combine(firstDirectory, "runtime", "guest-us.bin")).StartsWith("first:"), "previous version remains intact");
            Check(File.ReadAllText(Path.Combine(data, "player.sav")) == "SAVE MUST SURVIVE", "real profile save survives update");
            store.Install(second, "0.6.1", ReleaseStore.Sha256(second));
            Check(store.ActiveVersion == "0.6.1", "identical package can recover committed version");

            Rejected(store, second, "0.6.2", new string('0', 64), "tampered checksum");
            Rejected(store, first, "0.6.0", ReleaseStore.Sha256(first), "downgrade");
            Rejected(store, first, "0.6.1", ReleaseStore.Sha256(first), "replacement of immutable installed version");
            Rejected(store, second, "../0.6.2", ReleaseStore.Sha256(second), "unsafe version");
            Rejected(store, second, "0.0.0", ReleaseStore.Sha256(second), "zero version");
            string[] unsafeNames = { "../escaped.txt", "/absolute.txt", "C:/absolute.txt", "runtime\\evil.exe", "data/kh_com.ini",
                "practice-data/save.dat", "profile/data/save.dat", "runtime/game.gba", "runtime/player.sav", "runtime/slot.state1", "runtime/CON.txt", "runtime/file.txt:secret", "runtime/tail. " };
            int index = 0;
            foreach (string name in unsafeNames)
            {
                string zip = Archive("unsafe" + (++index), name, null, false);
                Rejected(store, zip, "0.6.2", ReleaseStore.Sha256(zip), name);
            }
            foreach (string extension in new[] { ".kst", ".khst", ".log" })
            {
                string zip = Archive("private" + extension, "runtime/player" + extension, null, false);
                Rejected(store, zip, "0.6.2", ReleaseStore.Sha256(zip), "personal " + extension);
            }
            string missing = Archive("missing", null, "runtime/SDL3.dll", false);
            Rejected(store, missing, "0.6.2", ReleaseStore.Sha256(missing), "missing runtime dependency");
            string duplicate = Archive("duplicate", "RUNTIME/SDL3.dll", null, false);
            Rejected(store, duplicate, "0.6.2", ReleaseStore.Sha256(duplicate), "case-insensitive duplicate entry");
            string linked = Archive("symlink", "runtime/link", null, true);
            Rejected(store, linked, "0.6.2", ReleaseStore.Sha256(linked), "symbolic link");
            string compressedLarge = Path.Combine(fixture, "oversized-expanded-file.zip");
            using (var file = File.Create(compressedLarge))
            using (var archive = new ZipArchive(file, ZipArchiveMode.Create))
            {
                var entry = archive.CreateEntry("runtime/oversized.bin", CompressionLevel.Fastest);
                using (Stream output = entry.Open())
                {
                    byte[] zeros = new byte[1024 * 1024];
                    for (int mb = 0; mb < 129; mb++) output.Write(zeros, 0, zeros.Length);
                }
            }
            Rejected(store, compressedLarge, "0.6.2", ReleaseStore.Sha256(compressedLarge), "oversized expanded file");
            string oversized = Path.Combine(fixture, "oversized-archive.zip");
            using (var output = File.Create(oversized)) output.SetLength(256L * 1024 * 1024 + 1);
            Rejected(store, oversized, "0.6.2", new string('0', 64), "oversized archive");
            Check(!File.Exists(Path.Combine(root, "escaped.txt")) && !File.Exists(Path.Combine(fixture, "escaped.txt")), "traversal writes no external file");
            Check(Directory.GetDirectories(Path.Combine(root, "versions"), ".stage-*").Length == 0, "rejected update staging is cleaned");
            bool setupNeeded = false;
            try { store.CheckForUpdate(""); } catch (InvalidOperationException error) { setupNeeded = error.Message.Contains("not connected"); }
            Check(setupNeeded, "unconnected GitHub has clear offline-play message");
            string equalJson = "{\"tag_name\":\"v0.6.1\",\"draft\":false,\"prerelease\":false,\"assets\":[]}";
            string olderJson = equalJson.Replace("0.6.1", "0.6.0");
            Check(store.ParseRelease("example/zwebti", equalJson) == null, "equal GitHub version returns no update (metadata fixture)");
            Check(store.ParseRelease("example/zwebti", olderJson) == null, "older GitHub version returns no update (metadata fixture)");
            string newerJson = "{\"tag_name\":\"v0.6.2\",\"draft\":false,\"prerelease\":false,\"assets\":[" +
                "{\"name\":\"Zwebti-Mix-0.6.2.zip\",\"state\":\"uploaded\",\"size\":123,\"browser_download_url\":\"https://github.com/example/zwebti/releases/download/v0.6.2/Zwebti-Mix-0.6.2.zip\"}," +
                "{\"name\":\"Zwebti-Mix-0.6.2.zip.sha256\",\"state\":\"uploaded\",\"size\":65,\"browser_download_url\":\"https://github.com/example/zwebti/releases/download/v0.6.2/Zwebti-Mix-0.6.2.zip.sha256\"}]}";
            var update = store.ParseRelease("example/zwebti", newerJson);
            Check(update != null && update.Version == "0.6.2" && update.Size == 123 && update.ArchiveName == "Zwebti-Mix-0.6.2.zip", "legacy GitHub assets remain compatible (metadata fixture)");
            string fullNameJson = newerJson.Replace("Zwebti-Mix-", "Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-");
            var fullNameUpdate = store.ParseRelease("example/zwebti", fullNameJson);
            Check(fullNameUpdate != null && fullNameUpdate.ArchiveName == "Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-0.6.2.zip", "full project release filename is selected");
            string bothNamesJson = fullNameJson.Substring(0, fullNameJson.Length - 2) + "," + newerJson.Substring(newerJson.IndexOf("[", StringComparison.Ordinal) + 1);
            var preferredUpdate = store.ParseRelease("example/zwebti", bothNamesJson);
            Check(preferredUpdate != null && preferredUpdate.ArchiveName == fullNameUpdate.ArchiveName, "full project filename takes precedence over legacy pair");
            bool incompletePreferred = false;
            try { store.ParseRelease("example/zwebti", bothNamesJson.Replace("Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-0.6.2.zip.sha256", "unrelated.sha256")); }
            catch (InvalidDataException) { incompletePreferred = true; }
            Check(incompletePreferred, "incomplete full-name pair cannot borrow a legacy checksum");
            bool duplicatePreferred = false;
            try { store.ParseRelease("example/zwebti", fullNameJson.Replace("]}", "," + fullNameJson.Substring(fullNameJson.IndexOf("[", StringComparison.Ordinal) + 1))); }
            catch (InvalidDataException) { duplicatePreferred = true; }
            Check(duplicatePreferred, "duplicate full-name assets rejected");
            bool unsafeAsset = false;
            try { store.ParseRelease("example/zwebti", newerJson.Replace("https://github.com/", "https://attacker.example/")); }
            catch (InvalidDataException) { unsafeAsset = true; }
            Check(unsafeAsset, "off-repository asset URL rejected (metadata fixture)");
            bool unsafeFullNameAsset = false;
            try { store.ParseRelease("example/zwebti", fullNameJson.Replace("https://github.com/", "https://attacker.example/")); }
            catch (InvalidDataException) { unsafeFullNameAsset = true; }
            Check(unsafeFullNameAsset, "full-name off-repository asset URL rejected");
            Check(File.ReadAllText(Path.Combine(data, "player.sav")) == "SAVE MUST SURVIVE", "all rejected updates preserve profile save");
            Console.WriteLine("Release store tests: " + passed + " passed. Live GitHub publishing/download remains untested.");
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
        finally { if (Directory.Exists(fixture)) Directory.Delete(fixture, true); }
    }
}
