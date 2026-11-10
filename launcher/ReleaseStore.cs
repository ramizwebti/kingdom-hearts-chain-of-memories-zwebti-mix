using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace ZwebtiMix
{
    public sealed class ReleaseInfo
    {
        public string Version { get; internal set; }
        public string ArchiveUrl { get; internal set; }
        public string ChecksumUrl { get; internal set; }
        public string ArchiveName { get; internal set; }
        public long Size { get; internal set; }
    }

    // Version folders are immutable. Only active-version.txt changes at commit;
    // profile/data and profile/practice-data never enter an update transaction.
    public sealed class ReleaseStore
    {
        const long MaxArchive = 256L * 1024 * 1024;
        const long MaxExpanded = 512L * 1024 * 1024;
        const long MaxFile = 128L * 1024 * 1024;
        const int MaxEntries = 20000;
        readonly string root;
        public string Root { get { return root; } }

        public ReleaseStore(string root)
        {
            if (String.IsNullOrWhiteSpace(root)) throw new ArgumentException("Choose a game installation folder.");
            this.root = Path.GetFullPath(root);
        }

        public string ActiveVersion
        {
            get
            {
                string pointer = Path.Combine(root, "active-version.txt");
                if (!File.Exists(pointer)) return null;
                string value = File.ReadAllText(pointer).Trim();
                ParseVersion(value);
                return value;
            }
        }

        public string ActiveDirectory
        {
            get
            {
                string version = ActiveVersion;
                return version == null ? null : Path.Combine(root, "versions", version);
            }
        }

        public static Version ParseVersion(string value)
        {
            Version parsed;
            if (value == null || !Regex.IsMatch(value, @"\A(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\z") ||
                !Version.TryParse(value, out parsed) || parsed <= new Version(0, 0, 0))
                throw new InvalidDataException("The update version is invalid.");
            return parsed;
        }

        public static string Sha256(string path)
        {
            using (FileStream stream = File.OpenRead(path))
                return Sha256(stream);
        }

        static string Sha256(Stream stream)
        {
            using (SHA256 hash = SHA256.Create())
                return BitConverter.ToString(hash.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
        }

        public void Install(string zipPath, string version, string expectedSha256)
        {
            Version targetVersion = ParseVersion(version);
            if (expectedSha256 == null || !Regex.IsMatch(expectedSha256, @"\A[0-9a-fA-F]{64}\z"))
                throw new InvalidDataException("The update checksum is missing or invalid.");
            if (!File.Exists(zipPath))
                throw new InvalidDataException("The update archive is missing or exceeds the size limit.");
            // Keep the same write-denying handle for hashing and extraction.
            // The archive cannot be swapped between verification and use.
            using (FileStream archiveStream = new FileStream(zipPath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                if (archiveStream.Length > MaxArchive)
                    throw new InvalidDataException("The update archive exceeds the size limit.");
                if (!String.Equals(Sha256(archiveStream), expectedSha256, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("The update download did not match its checksum. Your current game has not changed.");
                archiveStream.Position = 0;
                InstallVerified(archiveStream, version, targetVersion, expectedSha256);
            }
        }

        void InstallVerified(FileStream archiveStream, string version, Version targetVersion, string expectedSha256)
        {
            Directory.CreateDirectory(root);
            using (FileStream installLock = new FileStream(Path.Combine(root, "update.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None))
            {
                string active = ActiveVersion;
                if (active != null && targetVersion < ParseVersion(active))
                    throw new InvalidDataException("An older update cannot replace your current game.");
                string versions = Path.Combine(root, "versions");
                Directory.CreateDirectory(versions);
                string destination = Path.Combine(versions, version);
                if (Directory.Exists(destination))
                {
                    // Recover a complete version staged before a previous pointer commit.
                    string stamp = Path.Combine(destination, ".archive-sha256");
                    if (!File.Exists(stamp) || !String.Equals(File.ReadAllText(stamp).Trim(), expectedSha256, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("This version is already installed from a different package.");
                    ValidateRuntime(destination);
                    Commit(version);
                    return;
                }

                string stage = Path.Combine(versions, ".stage-" + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(stage);
                try
                {
                    using (ZipArchive archive = new ZipArchive(archiveStream, ZipArchiveMode.Read, true))
                    {
                        if (archive.Entries.Count > MaxEntries) throw new InvalidDataException("The update contains too many files.");
                        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                        long expanded = 0;
                        foreach (ZipArchiveEntry entry in archive.Entries)
                        {
                            string relative = ValidateEntry(entry);
                            if (!seen.Add(relative.TrimEnd('/'))) throw new InvalidDataException("The update contains duplicate paths.");
                            if (entry.Length < 0 || entry.Length > MaxFile || expanded > MaxExpanded - entry.Length)
                                throw new InvalidDataException("The extracted update exceeds the size limit.");
                            expanded += entry.Length;
                            string output = Path.GetFullPath(Path.Combine(stage, relative.Replace('/', Path.DirectorySeparatorChar)));
                            if (!output.StartsWith(stage + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                                throw new InvalidDataException("The update contains an unsafe path.");
                            if (relative.EndsWith("/", StringComparison.Ordinal)) { Directory.CreateDirectory(output); continue; }
                            Directory.CreateDirectory(Path.GetDirectoryName(output));
                            using (Stream source = entry.Open())
                            using (FileStream target = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                            {
                                CopyBounded(source, target, entry.Length);
                                if (target.Length != entry.Length) throw new InvalidDataException("An update file was truncated.");
                            }
                        }
                    }
                    ValidateRuntime(stage);
                    File.WriteAllText(Path.Combine(stage, ".archive-sha256"), expectedSha256.ToLowerInvariant(), new UTF8Encoding(false));
                    Directory.Move(stage, destination);
                    Commit(version);
                }
                finally
                {
                    if (Directory.Exists(stage)) Directory.Delete(stage, true);
                }
            }
        }

        static string ValidateEntry(ZipArchiveEntry entry)
        {
            string name = entry.FullName;
            if (String.IsNullOrEmpty(name) || name.IndexOf('\\') >= 0 || name.StartsWith("/", StringComparison.Ordinal) || name.IndexOf(':') >= 0)
                throw new InvalidDataException("The update contains an unsafe file name.");
            int kind = (entry.ExternalAttributes >> 16) & 0xf000;
            if (kind == 0xa000 || (entry.ExternalAttributes & 0x400) != 0)
                throw new InvalidDataException("Links are not allowed in updates.");
            string[] pieces = name.TrimEnd('/').Split('/');
            foreach (string part in pieces)
            {
                if (part.Length == 0 || part == "." || part == ".." || part.EndsWith(" ", StringComparison.Ordinal) || part.EndsWith(".", StringComparison.Ordinal) ||
                    part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || Regex.IsMatch(part, @"\A(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|\z)", RegexOptions.IgnoreCase))
                    throw new InvalidDataException("The update contains an unsafe file name.");
                if (String.Equals(part, "data", StringComparison.OrdinalIgnoreCase) || String.Equals(part, "practice-data", StringComparison.OrdinalIgnoreCase) ||
                    String.Equals(part, "profile", StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Updates cannot include player data.");
            }
            string extension = Path.GetExtension(name).ToLowerInvariant();
            if (extension == ".gba" || extension == ".sav" || extension == ".srm" || extension == ".state" || extension == ".ss0" || extension == ".dmp" ||
                extension == ".kst" || extension == ".khst" || extension == ".log" ||
                Regex.IsMatch(extension, @"\A\.(sgm|ss|state)[0-9]*\z"))
                throw new InvalidDataException("Updates cannot include game dumps or saves.");
            if (String.Equals(name, ".archive-sha256", StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("The update contains a reserved metadata name.");
            return name;
        }

        static void ValidateRuntime(string folder)
        {
            foreach (string name in new[] { "launch.ps1", "runtime/rekhcom64.exe", "runtime/guest-us.bin", "runtime/SDL3.dll" })
            {
                string path = Path.Combine(folder, name.Replace('/', Path.DirectorySeparatorChar));
                if (!File.Exists(path) || new FileInfo(path).Length == 0) throw new InvalidDataException("The update is missing a required game file: " + name);
            }
        }

        void Commit(string version)
        {
            string pointer = Path.Combine(root, "active-version.txt");
            string temporary = Path.Combine(root, ".active-" + Guid.NewGuid().ToString("N"));
            try
            {
                using (FileStream output = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    byte[] bytes = Encoding.ASCII.GetBytes(version + "\r\n");
                    output.Write(bytes, 0, bytes.Length);
                    output.Flush(true);
                }
                if (File.Exists(pointer)) File.Replace(temporary, pointer, null);
                else File.Move(temporary, pointer);
            }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }

        public ReleaseInfo GetLatest(string repository) { return CheckForUpdate(repository); }

        public ReleaseInfo CheckForUpdate(string repository)
        {
            if (repository == null || !Regex.IsMatch(repository, @"\A[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})/[A-Za-z0-9_.-]{1,100}\z") || repository.EndsWith("/.git", StringComparison.Ordinal))
                throw new InvalidOperationException("GitHub updates are not connected yet. The game can still be played offline.");
            byte[] json = DownloadBytes(new Uri("https://api.github.com/repos/" + repository + "/releases/latest"), 2 * 1024 * 1024, true);
            return ParseRelease(repository, Encoding.UTF8.GetString(json));
        }

        // Metadata fixtures exercise parsing only; live downloads remain HTTPS.
        internal ReleaseInfo ParseRelease(string repository, string json)
        {
            var serializer = new JavaScriptSerializer { MaxJsonLength = 2 * 1024 * 1024, RecursionLimit = 32 };
            var release = serializer.DeserializeObject(json) as Dictionary<string, object>;
            if (release == null || GetBool(release, "draft") || GetBool(release, "prerelease")) throw new InvalidDataException("GitHub did not return a stable published release.");
            string tag = GetString(release, "tag_name");
            string version = tag.StartsWith("v", StringComparison.Ordinal) ? tag.Substring(1) : tag;
            Version latest = ParseVersion(version);
            string activeVersion = ActiveVersion;
            if (activeVersion != null && latest <= ParseVersion(activeVersion)) return null;
            string archiveName = "Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-" + version + ".zip";
            string legacyName = "Zwebti-Mix-" + version + ".zip";
            Dictionary<string, object> archive = null, checksum = null;
            Dictionary<string, object> legacyArchive = null, legacyChecksum = null;
            object assetsObject;
            var assets = release.TryGetValue("assets", out assetsObject) ? assetsObject as object[] : null;
            if (assets == null) throw new InvalidDataException("The GitHub release has no update assets.");
            foreach (object item in assets)
            {
                var asset = item as Dictionary<string, object>;
                if (asset == null || GetString(asset, "state") != "uploaded") continue;
                string name = GetString(asset, "name");
                if (name == archiveName) { if (archive != null) throw new InvalidDataException("Duplicate update assets."); archive = asset; }
                if (name == archiveName + ".sha256") { if (checksum != null) throw new InvalidDataException("Duplicate update checksums."); checksum = asset; }
                if (name == legacyName) { if (legacyArchive != null) throw new InvalidDataException("Duplicate update assets."); legacyArchive = asset; }
                if (name == legacyName + ".sha256") { if (legacyChecksum != null) throw new InvalidDataException("Duplicate update checksums."); legacyChecksum = asset; }
            }
            // Existing releases retain their original filenames. A published
            // full-name pair takes precedence and may never mix with an alias.
            if (archive == null && checksum == null) {
                archiveName = legacyName;
                archive = legacyArchive;
                checksum = legacyChecksum;
            }
            if (archive == null || checksum == null) throw new InvalidDataException("The latest release does not contain a complete Windows update.");
            long size = Convert.ToInt64(archive["size"], CultureInfo.InvariantCulture);
            if (size <= 0 || size > MaxArchive) throw new InvalidDataException("The update size is invalid.");
            string archiveUrl = GetString(archive, "browser_download_url"), checksumUrl = GetString(checksum, "browser_download_url");
            string prefix = "https://github.com/" + repository + "/releases/download/" + Uri.EscapeDataString(tag) + "/";
            if (!String.Equals(archiveUrl, prefix + archiveName, StringComparison.OrdinalIgnoreCase) ||
                !String.Equals(checksumUrl, prefix + archiveName + ".sha256", StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("The update assets point outside this GitHub release.");
            return new ReleaseInfo { Version = version, ArchiveUrl = archiveUrl, ChecksumUrl = checksumUrl, ArchiveName = archiveName, Size = size };
        }

        public void DownloadAndInstall(ReleaseInfo release)
        {
            if (release == null) throw new ArgumentNullException("release");
            ParseVersion(release.Version);
            string checksumText = Encoding.UTF8.GetString(DownloadBytes(new Uri(release.ChecksumUrl), 4096, false)).Trim();
            Match checksum = Regex.Match(checksumText, @"\A([0-9a-fA-F]{64})(?:[ \t]+\*?([^\r\n]+))?\z");
            if (!checksum.Success || (checksum.Groups[2].Success && checksum.Groups[2].Value != release.ArchiveName))
                throw new InvalidDataException("The published update checksum is invalid.");
            Directory.CreateDirectory(root);
            string download = Path.Combine(root, ".download-" + Guid.NewGuid().ToString("N") + ".zip");
            try
            {
                using (Stream input = OpenDownload(new Uri(release.ArchiveUrl), false))
                using (FileStream output = new FileStream(download, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    CopyBounded(input, output, MaxArchive);
                    if (output.Length != release.Size) throw new InvalidDataException("The update download was truncated.");
                }
                Install(download, release.Version, checksum.Groups[1].Value);
            }
            finally { if (File.Exists(download)) File.Delete(download); }
        }

        static bool GetBool(Dictionary<string, object> value, string name)
        {
            object result;
            return value.TryGetValue(name, out result) && result is bool && (bool)result;
        }

        static string GetString(Dictionary<string, object> value, string name)
        {
            object result;
            if (!value.TryGetValue(name, out result) || !(result is string)) throw new InvalidDataException("GitHub returned incomplete update information.");
            return (string)result;
        }

        static byte[] DownloadBytes(Uri url, int limit, bool api)
        {
            using (Stream input = OpenDownload(url, api))
            using (var output = new MemoryStream()) { CopyBounded(input, output, limit); return output.ToArray(); }
        }

        static Stream OpenDownload(Uri url, bool api)
        {
            // Never send credentials. Validate every redirect before connecting.
            ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
            for (int redirects = 0; redirects <= 5; redirects++)
            {
                string host = url.DnsSafeHost.ToLowerInvariant();
                if (url.Scheme != "https" || !url.IsDefaultPort || !String.IsNullOrEmpty(url.UserInfo) ||
                    !(host == "api.github.com" || host == "github.com" || host == "objects.githubusercontent.com" ||
                      host == "release-assets.githubusercontent.com" || host == "github-releases.githubusercontent.com"))
                    throw new InvalidDataException("The update download left GitHub's secure servers.");
                var request = (HttpWebRequest)WebRequest.Create(url);
                request.AllowAutoRedirect = false;
                request.Timeout = 30000;
                request.ReadWriteTimeout = 30000;
                request.UserAgent = "Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Launcher/0.10.1";
                request.Accept = api ? "application/vnd.github+json" : "application/octet-stream";
                request.Headers["X-GitHub-Api-Version"] = "2026-03-10";
                request.Credentials = null;
                var response = (HttpWebResponse)request.GetResponse();
                int code = (int)response.StatusCode;
                if (code >= 300 && code <= 399)
                {
                    string location = response.Headers["Location"];
                    response.Dispose();
                    Uri next;
                    if (String.IsNullOrEmpty(location) || !Uri.TryCreate(url, location, out next)) throw new InvalidDataException("GitHub returned an invalid redirect.");
                    url = next;
                    continue;
                }
                if (code != 200) { response.Dispose(); throw new WebException("GitHub could not provide the update."); }
                return new ResponseStream(response);
            }
            throw new InvalidDataException("GitHub returned too many download redirects.");
        }

        static void CopyBounded(Stream input, Stream output, long limit)
        {
            byte[] buffer = new byte[65536];
            long total = 0;
            int count;
            while ((count = input.Read(buffer, 0, buffer.Length)) != 0)
            {
                if (total > limit - count) throw new InvalidDataException("The update exceeds its allowed size.");
                output.Write(buffer, 0, count);
                total += count;
            }
        }

        sealed class ResponseStream : Stream
        {
            readonly HttpWebResponse response;
            readonly Stream inner;
            public ResponseStream(HttpWebResponse response) { this.response = response; inner = response.GetResponseStream(); }
            public override bool CanRead { get { return inner.CanRead; } }
            public override bool CanSeek { get { return false; } }
            public override bool CanWrite { get { return false; } }
            public override long Length { get { return inner.Length; } }
            public override long Position { get { return inner.Position; } set { throw new NotSupportedException(); } }
            public override int Read(byte[] buffer, int offset, int count) { return inner.Read(buffer, offset, count); }
            public override void Flush() { }
            public override long Seek(long offset, SeekOrigin origin) { throw new NotSupportedException(); }
            public override void SetLength(long value) { throw new NotSupportedException(); }
            public override void Write(byte[] buffer, int offset, int count) { throw new NotSupportedException(); }
            protected override void Dispose(bool disposing) { if (disposing) { inner.Dispose(); response.Dispose(); } base.Dispose(disposing); }
        }
    }
}
