using System;
using System.Collections.Generic;
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
using System.Windows.Shapes;

namespace NidhogEditor.GameProject
{
    /// <summary>
    /// ProjectBrowsweDialg.xaml 的交互逻辑
    /// </summary>
    public partial class ProjectBrowsweDialg : Window
    {
        public ProjectBrowsweDialg()
        {
            InitializeComponent();
            Loaded += OnProjectBrowsweDialogLoaded;
        }

        private void OnProjectBrowsweDialogLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnProjectBrowsweDialogLoaded;
            if (!OpenProject.Projects.Any())
            {
                OpenProjectButton.IsEnabled = false;
                openProjectView.Visibility = Visibility.Hidden;
                OnToggleButton_Click(CreatProjectButton, new RoutedEventArgs());
            }
        }

        private void OnToggleButton_Click(object sender,RoutedEventArgs e)
        {
            if(sender == OpenProjectButton)
            {
                if(CreatProjectButton.IsChecked == true)
                {
                    CreatProjectButton.IsChecked = false;
                    BrowswerContent.Margin = new Thickness(0);
                }
                OpenProjectButton.IsChecked = true;
            }
            else
            {
                if (OpenProjectButton.IsChecked == true)
                {
                    OpenProjectButton.IsChecked = false;
                    BrowswerContent.Margin = new Thickness(-800,0,0,0);
                }
                CreatProjectButton.IsChecked = true;
            }
        }
    }
}
