using NidhogEditor.Content;
using NidhogEditor.GameProject;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NidhogEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static string NidhogPath { get; private set; }
        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainwindowLoaded;
            Closing += OnMainwindowClosing;
        }

        private void OnMainwindowClosing(object sender, CancelEventArgs e)
        {
            if (DataContext == null)
            {
                e.Cancel = true;
                Application.Current.MainWindow.Hide();
                OpenProjectBrowserDialog();
                if(DataContext != null)
                {
                    Application.Current.MainWindow.Show();
                }
            }
            else
            {
                Closing -= OnMainwindowClosing;
                Project.Current?.Unload();
                DataContext = null;
            }
        }

        private void OnMainwindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnMainwindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }


        private void GetEnginePath()
        {
            var nidhogPath = Environment.GetEnvironmentVariable("NIDHOG_ENGINE", EnvironmentVariableTarget.User);
            if (nidhogPath == null || !Directory.Exists(System.IO.Path.Combine(nidhogPath, @"Engine\EngineAPI")))
            {
                var dlg = new EnginePathDialog();
                if (dlg.ShowDialog() == true)
                {
                    NidhogPath = dlg.NidhogPath;
                    Environment.SetEnvironmentVariable("PRIMAL_ENGINE", NidhogPath.ToUpper(), EnvironmentVariableTarget.User);
                }
                else
                {
                    Application.Current.Shutdown();
                }
            }
            else
            {
                NidhogPath = nidhogPath;
            }
        }

        private void OpenProjectBrowserDialog()
        {
            var projectBrowser = new ProjectBrowsweDialg();
            if (projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)
            {
                Application.Current.Shutdown();
            }
            else
            {
                Project.Current?.Unload();
                var project = projectBrowser.DataContext as Project;
                Debug.Assert(project != null);
                AssetRegistry.Reset(project.ContentPath);
                DataContext = project;
            }
        }
    }
}
