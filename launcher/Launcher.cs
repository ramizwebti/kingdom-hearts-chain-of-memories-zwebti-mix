using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Threading;
using System.Windows.Forms;
using System.Web.Script.Serialization;

[assembly: AssemblyTitle("Kingdom Hearts Chain of Memories: Zwebti Mix")]
[assembly: AssemblyProduct("Kingdom Hearts Chain of Memories: Zwebti Mix")]
[assembly: AssemblyVersion("0.11.1.0")]
[assembly: AssemblyFileVersion("0.11.1.0")]

namespace ZwebtiMix {
    internal static class Program {
        internal const string ProjectTitle = "Kingdom Hearts Chain of Memories: Zwebti Mix";
        internal const string SeedVersion = "0.11.1";
        internal const string UpdateRepository = "ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix";
        [STAThread]
        private static int Main(string[] args) {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            // The installer has its own release-store lock. Isolated build
            // checks and version upgrades must also work while the UI is open.
            if (args.Length == 3 && args[0] == "--verify-install") {
                try {
                    ReleaseStore target = new ReleaseStore(Path.GetFullPath(args[1]));
                    InstallSeed(target);
                    File.WriteAllText(args[2], target.ActiveVersion);
                    return 0;
                } catch { return 1; }
            }
            bool created;
            using (Mutex singleton = new Mutex(true, "Local\\KingdomHeartsChainOfMemoriesZwebtiMixLauncher", out created)) {
                if (!created) { MessageBox.Show(ProjectTitle + " is already open.", ProjectTitle); return 0; }
                try {
                    string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Kingdom Hearts Chain of Memories - Zwebti Mix");
                    Application.Run(new LauncherForm(new ReleaseStore(root)));
                    return 0;
                } catch (Exception error) {
                    MessageBox.Show(error.Message, ProjectTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return 1;
                }
            }
        }

        internal static byte[] Resource(string name) {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name)) {
                if (stream == null) throw new IOException("The launcher is incomplete. Please keep the original Kingdom Hearts Chain of Memories - Zwebti Mix.exe.");
                using (MemoryStream buffer = new MemoryStream()) { stream.CopyTo(buffer); return buffer.ToArray(); }
            }
        }

        internal static void InstallSeed(ReleaseStore store) {
            if (!String.IsNullOrEmpty(store.ActiveVersion) &&
                ReleaseStore.ParseVersion(store.ActiveVersion) >= ReleaseStore.ParseVersion(SeedVersion)) return;
            Directory.CreateDirectory(store.Root);
            string path = Path.Combine(store.Root, "seed-" + Guid.NewGuid().ToString("N") + ".zip");
            try {
                byte[] bytes = Resource("release.zip");
                File.WriteAllBytes(path, bytes);
                string hash;
                using (SHA256 sha = SHA256.Create()) { hash = BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant(); }
                store.Install(path, SeedVersion, hash);
            } finally { if (File.Exists(path)) File.Delete(path); }
        }
    }

    internal sealed class LauncherForm : Form {
        private readonly ReleaseStore store;
        private readonly Button story = new Button(), practice = new Button(), update = new Button();
        private readonly ComboBox presets = new ComboBox();
        private readonly Label status = new Label(), version = new Label();
        private bool busy;

        internal LauncherForm(ReleaseStore value) {
            store = value;
            Text = Program.ProjectTitle;
            ClientSize = new Size(660, 550);
            StartPosition = FormStartPosition.CenterScreen;
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            BackColor = Color.FromArgb(247, 246, 242);
            ForeColor = Color.FromArgb(29, 36, 48);
            Font = new Font("Segoe UI", 10);
            using (MemoryStream icon = new MemoryStream(Program.Resource("heart.ico"))) { Icon = new Icon(icon); }
            PictureBox logo = new PictureBox();
            logo.SetBounds(30, 12, 600, 300);
            logo.SizeMode = PictureBoxSizeMode.Zoom;
            using (MemoryStream pixels = new MemoryStream(Program.Resource("logo.png"))) { using (Image image = Image.FromStream(pixels)) { logo.Image = new Bitmap(image); } }
            Controls.Add(logo);
            story.Text = "Play Story";
            story.SetBounds(38, 324, 282, 44);
            story.Click += delegate { Launch(""); };
            Controls.Add(story);
            practice.Text = "Practice Arena";
            practice.SetBounds(340, 324, 282, 44);
            practice.Click += delegate { Launch(new string[] {"training", "crowd", "mixed", "axel", "jafar"}[presets.SelectedIndex]); };
            Controls.Add(practice);
            Label hint = new Label();
            hint.Text = "Story and practice keep separate saves.";
            hint.SetBounds(38, 382, 282, 42);
            hint.ForeColor = Color.FromArgb(83, 90, 100);
            Controls.Add(hint);
            presets.DropDownStyle = ComboBoxStyle.DropDownList;
            presets.Items.AddRange(new object[] {"Training", "Shadow crowd", "Mixed encounter", "Axel", "Jafar"});
            presets.SelectedIndex = 0;
            presets.SetBounds(340, 382, 282, 30);
            Controls.Add(presets);
            update.Text = "Check for updates";
            update.SetBounds(38, 440, 190, 36);
            update.Click += delegate { CheckUpdates(); };
            Controls.Add(update);
            Button saves = new Button();
            saves.Text = "Open saves";
            saves.SetBounds(242, 440, 125, 36);
            saves.Click += delegate {
                string profile = Path.Combine(store.Root, "profile");
                Directory.CreateDirectory(profile);
                Process.Start(new ProcessStartInfo("explorer.exe", Quote(profile)) { UseShellExecute = false });
            };
            Controls.Add(saves);
            version.TextAlign = ContentAlignment.MiddleRight;
            version.SetBounds(494, 440, 128, 36);
            version.ForeColor = hint.ForeColor;
            Controls.Add(version);
            status.SetBounds(38, 494, 584, 40);
            status.Font = new Font("Segoe UI", 9);
            Controls.Add(status);
            Shown += delegate {
                Run("Preparing your game...", delegate { Program.InstallSeed(store); }, delegate { status.Text = "Ready to play."; });
            };
            FormClosing += delegate(object sender, FormClosingEventArgs e) { if (busy) e.Cancel = true; };
            AcceptButton = story;
        }

        private void Run(string message, Action work, Action done) {
            if (busy) return;
            busy = true;
            story.Enabled = practice.Enabled = update.Enabled = presets.Enabled = false;
            status.Text = message;
            BackgroundWorker worker = new BackgroundWorker();
            worker.DoWork += delegate { work(); };
            worker.RunWorkerCompleted += delegate(object sender, RunWorkerCompletedEventArgs result) {
                busy = false;
                story.Enabled = practice.Enabled = update.Enabled = presets.Enabled = true;
                version.Text = "v" + store.ActiveVersion;
                if (result.Error != null) {
                    status.Text = "Could not finish. Your existing game data is preserved.";
                    MessageBox.Show(this, result.Error.Message, Program.ProjectTitle, MessageBoxButtons.OK, MessageBoxIcon.Information);
                } else { done(); }
                worker.Dispose();
            };
            worker.RunWorkerAsync();
        }

        private void Launch(string preset) {
            Run("Game is running. Close it to return here.", delegate {
                string script = Path.Combine(store.ActiveDirectory, "launch.ps1");
                string ps = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe");
                ProcessStartInfo start = new ProcessStartInfo(ps);
                start.Arguments = "-NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File " + Quote(script) +
                    " -ProfileRoot " + Quote(Path.Combine(store.Root, "profile")) +
                    (preset.Length == 0 ? " -Story" : " -Practice " + preset);
                start.UseShellExecute = false;
                start.CreateNoWindow = true;
                start.WorkingDirectory = store.ActiveDirectory;
                using (Process game = Process.Start(start)) {
                    game.WaitForExit();
                    if (game.ExitCode != 0) throw new IOException("The game could not start. Check the message from the game launcher and try again.");
                }
            }, delegate { status.Text = "Ready to play."; });
        }

        private void CheckUpdates() {
            ReleaseInfo next = null;
            string repository = Program.UpdateRepository;
            string config = Path.Combine(store.Root, "updates.json");
            if (File.Exists(config)) {
                try {
                    var values = new JavaScriptSerializer().Deserialize<System.Collections.Generic.Dictionary<string, string>>(File.ReadAllText(config));
                    if (values != null && values.ContainsKey("repository") &&
                        !String.IsNullOrWhiteSpace(values["repository"]) &&
                        !String.Equals(values["repository"], Program.UpdateRepository, StringComparison.OrdinalIgnoreCase)) {
                        MessageBox.Show(this, "The update settings point to a different project. This launcher updates Kingdom Hearts Chain of Memories: Zwebti Mix only.", Program.ProjectTitle);
                        return;
                    }
                } catch (Exception error) { MessageBox.Show(this, "The update settings could not be read: " + error.Message, Program.ProjectTitle); return; }
            }
            Run("Checking for updates...", delegate {
                next = store.CheckForUpdate(repository);
                if (next != null) store.DownloadAndInstall(next);
            }, delegate {
                status.Text = next == null ? "You have the latest version." : "Updated to " + store.ActiveVersion + ". Ready to play.";
            });
        }

        private static string Quote(string value) {
            // All paths end with a filename or a known directory component, never a trailing slash.
            if (value.IndexOf('"') >= 0 || value.EndsWith("\\")) throw new ArgumentException("Unsupported path.");
            return "\"" + value + "\"";
        }
    }
}
