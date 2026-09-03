using System;
using System.Drawing;
using System.Threading.Tasks;
using System.Windows.Forms;
using FrontierLoader.Services;
using Guna.UI2.WinForms;

namespace FrontierLoader
{
    internal sealed class MainForm : Form
    {
        readonly Color _bg = Color.FromArgb(10, 10, 10);
        readonly Color _panel = Color.FromArgb(17, 16, 17);
        readonly Color _accent = Color.FromArgb(56, 189, 248);
        readonly Color _accentBtn = Color.FromArgb(48, 104, 194);
        readonly Color _dim = Color.FromArgb(128, 128, 128);
        readonly Color _ok = Color.FromArgb(109, 179, 91);

        readonly Guna2BorderlessForm _borderless;
        readonly Guna2ControlBox _closeBox;
        readonly Label _logo;
        readonly Label _hello;
        readonly Guna2TextBox _keyBox;
        readonly Guna2Button _activateBtn;
        readonly Label _errorLabel;
        readonly Guna2ProgressBar _progress;

        readonly Guna2ShadowPanel _launchPanel;
        readonly Label _productLabel;
        readonly Label _statusLabel;
        readonly Label _expireLabel;
        readonly Guna2ComboBox _modeCombo;
        readonly Guna2Button _launchBtn;
        readonly Guna2Button _updateBtn;
        readonly LinkLabel _siteLink;

        string _token = "";
        string _hwid = "";
        bool _licensed;
        bool _busy;

        public MainForm()
        {
            Text = "FRONTIER";
            Size = new Size(420, 300);
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = _bg;
            FormBorderStyle = FormBorderStyle.None;
            Opacity = 0;

            _borderless = new Guna2BorderlessForm { ContainerControl = this, BorderRadius = 12, ShadowColor = Color.Black };
            _closeBox = new Guna2ControlBox
            {
                Anchor = AnchorStyles.Top | AnchorStyles.Right,
                FillColor = _bg,
                IconColor = Color.White,
                Location = new Point(Width - 46, 4),
                Size = new Size(36, 28)
            };

            _logo = new Label
            {
                Text = "FRONTIER",
                ForeColor = Color.White,
                Font = new Font("Segoe UI Semibold", 16f, FontStyle.Bold),
                AutoSize = true,
                Location = new Point(24, 16)
            };

            _hello = new Label
            {
                Text = "Welcome back",
                ForeColor = _dim,
                Font = new Font("Segoe UI", 11f),
                AutoSize = true,
                Location = new Point(28, 72)
            };

            _keyBox = new Guna2TextBox
            {
                PlaceholderText = "License key (FRTR-XXXX-XXXX-XXXX)",
                Location = new Point(28, 118),
                Size = new Size(364, 36),
                BorderRadius = 8,
                FillColor = Color.FromArgb(18, 18, 20),
                ForeColor = Color.White,
                PlaceholderForeColor = Color.FromArgb(90, 90, 96)
            };

            _activateBtn = new Guna2Button
            {
                Text = "Activate",
                Location = new Point(28, 168),
                Size = new Size(364, 38),
                BorderRadius = 8,
                FillColor = _accentBtn,
                ForeColor = Color.White,
                Font = new Font("Segoe UI Semibold", 10f)
            };
            _activateBtn.Click += async (_, __) => await ActivateAsync();

            _errorLabel = new Label
            {
                ForeColor = Color.FromArgb(255, 120, 110),
                Font = new Font("Segoe UI", 9f),
                AutoSize = false,
                Size = new Size(364, 36),
                Location = new Point(28, 210),
                Visible = false
            };

            _progress = new Guna2ProgressBar
            {
                Location = new Point(72, 206),
                Size = new Size(276, 6),
                Visible = false,
                ProgressColor = _accentBtn,
                ProgressColor2 = _accentBtn,
                FillColor = Color.FromArgb(24, 24, 28)
            };

            _launchPanel = new Guna2ShadowPanel
            {
                Location = new Point(22, 60),
                Size = new Size(376, 210),
                FillColor = _panel,
                ShadowColor = Color.FromArgb(21, 19, 21),
                Radius = 10,
                Visible = false
            };

            _productLabel = new Label
            {
                Text = "FRONTIER",
                ForeColor = _accent,
                Font = new Font("Segoe UI Semibold", 13f, FontStyle.Bold),
                AutoSize = true,
                Location = new Point(16, 14)
            };

            _statusLabel = new Label
            {
                Text = "Status: Ready",
                ForeColor = _ok,
                Font = new Font("Segoe UI", 9f),
                AutoSize = true,
                Location = new Point(16, 44)
            };

            _expireLabel = new Label
            {
                Text = "Licensed",
                ForeColor = _dim,
                Font = new Font("Segoe UI", 9f),
                AutoSize = true,
                Location = new Point(16, 64)
            };

            _modeCombo = new Guna2ComboBox
            {
                Location = new Point(16, 96),
                Size = new Size(344, 36),
                BorderRadius = 8,
                FillColor = Color.FromArgb(14, 14, 16),
                ForeColor = Color.White,
                BackColor = Color.Transparent
            };
            _modeCombo.Items.Add("Usermode (Recommended)");
            _modeCombo.Items.Add("Kernel (Advanced)");
            _modeCombo.SelectedIndex = LauncherService.LoadMode() == 1 ? 1 : 0;

            _launchBtn = new Guna2Button
            {
                Text = "Launch",
                Location = new Point(16, 146),
                Size = new Size(168, 38),
                BorderRadius = 8,
                FillColor = _accentBtn,
                ForeColor = Color.White
            };
            _launchBtn.Click += async (_, __) => await LaunchAsync();

            _updateBtn = new Guna2Button
            {
                Text = "Update",
                Location = new Point(192, 146),
                Size = new Size(168, 38),
                BorderRadius = 8,
                FillColor = Color.FromArgb(28, 28, 32),
                ForeColor = Color.White
            };
            _updateBtn.Click += async (_, __) => await UpdateOnlyAsync();

            _siteLink = new LinkLabel
            {
                Text = "ahead.best",
                LinkColor = _accent,
                ActiveLinkColor = Color.White,
                AutoSize = true,
                Location = new Point(16, 192)
            };
            _siteLink.LinkClicked += (_, __) =>
                System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(FrontierConfig.SiteUrl) { UseShellExecute = true });

            _launchPanel.Controls.AddRange(new Control[]
            {
                _productLabel, _statusLabel, _expireLabel, _modeCombo,
                _launchBtn, _updateBtn, _siteLink
            });

            Controls.AddRange(new Control[]
            {
                _logo, _hello, _keyBox, _activateBtn, _errorLabel, _progress,
                _launchPanel, _closeBox
            });

            Load += async (_, __) =>
            {
                FadeIn();
                _hwid = LicenseClient.GetHwid();
                _token = LicenseClient.OpenToken;
                _licensed = true;
                _keyBox.Visible = false;
                _activateBtn.Visible = false;
                _hello.Text = "Open source — no key required";
                ShowLaunchPanel("Open source — no key required.");
                await Task.CompletedTask;
            };

            Shown += (_, __) => _closeBox.BringToFront();
        }

        void FadeIn()
        {
            var t = new Timer { Interval = 16 };
            t.Tick += (_, __) =>
            {
                Opacity += 0.06;
                if (Opacity >= 1)
                {
                    Opacity = 1;
                    t.Stop();
                    t.Dispose();
                }
            };
            t.Start();
        }

        async Task TryResumeSessionAsync()
        {
            SetBusy(true, "Checking license...");
            var res = await LicenseClient.ValidateTokenAsync(_token, _hwid);
            SetBusy(false);
            if (res.Ok)
            {
                _licensed = res.Licensed;
                ShowLaunchPanel(res.Message);
            }
        }

        async Task ActivateAsync()
        {
            if (_busy) return;
            HideError();
            SetBusy(true, "Activating...");
            var res = await LicenseClient.ActivateKeyAsync(_keyBox.Text, _hwid);
            SetBusy(false);
            if (!res.Ok)
            {
                ShowError(res.Message);
                return;
            }
            _token = res.Token;
            _licensed = res.Licensed;
            if (!_licensed)
            {
                ShowError("Key accepted but FRONTIER is not licensed on this account.");
                return;
            }
            await AnimateToLaunchAsync(res.Message);
        }

        async Task AnimateToLaunchAsync(string msg)
        {
            _progress.Visible = true;
            _progress.Value = 0;
            for (int i = 0; i <= 100; i += 4)
            {
                _progress.Value = Math.Min(100, i);
                await Task.Delay(12);
            }
            _progress.Visible = false;
            ShowLaunchPanel(msg);
        }

        void ShowLaunchPanel(string msg)
        {
            foreach (Control c in Controls)
            {
                if (c == _launchPanel || c == _closeBox || c == _logo) continue;
                c.Visible = false;
            }
            _launchPanel.Visible = true;
            _statusLabel.Text = _licensed ? "Status: Licensed" : "Status: Unlicensed";
            _statusLabel.ForeColor = _licensed ? _ok : Color.FromArgb(255, 170, 100);
            _expireLabel.Text = msg;
            _launchBtn.Enabled = _licensed;
        }

        async Task LaunchAsync()
        {
            if (_busy || !_licensed) return;
            var mode = _modeCombo.SelectedIndex == 1 ? 1 : 0;
            try
            {
                SetBusy(true, "Preparing...");
                var manifest = await UpdateClient.FetchManifestAsync();
                if (manifest == null) throw new InvalidOperationException("Could not read update manifest.");

                var progress = new Progress<(float pct, string msg)>(p =>
                {
                    _progress.Visible = true;
                    _progress.Value = Math.Max(0, Math.Min(100, (int)(p.pct * 100)));
                    _statusLabel.Text = p.msg;
                });

                await UpdateClient.EnsureUpdatedAsync(manifest, mode, progress, default);
                var hwid = string.IsNullOrWhiteSpace(_hwid) ? LicenseClient.GetHwid() : _hwid;
                LauncherService.Launch(mode, _token, hwid);
                await Task.Delay(400);
                Close();
            }
            catch (Exception ex)
            {
                ShowError(ex.Message);
            }
            finally
            {
                SetBusy(false);
                _progress.Visible = false;
                _statusLabel.Text = _licensed ? "Status: Licensed" : "Status: Unlicensed";
            }
        }

        async Task UpdateOnlyAsync()
        {
            if (_busy) return;
            try
            {
                SetBusy(true, "Checking updates...");
                var manifest = await UpdateClient.FetchManifestAsync();
                if (manifest == null) throw new InvalidOperationException("Could not read update manifest.");
                var mode = _modeCombo.SelectedIndex == 1 ? 1 : 0;
                var progress = new Progress<(float pct, string msg)>(p =>
                {
                    _progress.Visible = true;
                    _progress.Value = Math.Max(0, Math.Min(100, (int)(p.pct * 100)));
                    _statusLabel.Text = p.msg;
                });
                await UpdateClient.EnsureUpdatedAsync(manifest, mode, progress, default);
                _statusLabel.Text = "Updated to " + manifest.Display;
            }
            catch (Exception ex)
            {
                ShowError(ex.Message);
            }
            finally
            {
                SetBusy(false);
                _progress.Visible = false;
            }
        }

        void SetBusy(bool busy, string? status = null)
        {
            _busy = busy;
            _activateBtn.Enabled = !busy;
            _launchBtn.Enabled = !busy && _licensed;
            _updateBtn.Enabled = !busy;
            _keyBox.Enabled = !busy;
            if (status != null && _launchPanel.Visible)
                _statusLabel.Text = status;
        }

        void ShowError(string msg)
        {
            _errorLabel.Text = msg;
            _errorLabel.Visible = true;
        }

        void HideError() => _errorLabel.Visible = false;
    }
}
