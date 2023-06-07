using NidhogEditor.GameProject;
using System;
using System.Collections.Generic;
using System.ComponentModel;
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
using System.IO;

namespace NidhogEditor
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public static string NidhogPath { get; private set; } = @"D:\Nidhog";
        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainwindowLoaded;
            Closing += OnMainwindowClosing;
        }

        private void OnMainwindowClosing(object sender, CancelEventArgs e)
        {
            Closing -= OnMainwindowClosing;
            Project.Current?.Unload();
        }

        private void OnMainwindowLoaded(object sender,RoutedEventArgs e)
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
            if(projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)
            {
                Application.Current.Shutdown();
            }
            else
            {
                Project.Current?.Unload();
                DataContext = projectBrowser.DataContext;
            }
        }
    }
}
