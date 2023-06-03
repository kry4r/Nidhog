using NidhogEditor.Components;
using NidhogEditor.GameProject;
using NidhogEditor.Utilities;
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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NidhogEditor.Editors
{
    /// <summary>
    /// ProjectLayoutView.xaml 的交互逻辑
    /// </summary>
    public partial class ProjectLayoutView : UserControl
    {
        public ProjectLayoutView()
        {
            InitializeComponent();
        }


        private void OnAddGameEntity_Button_Click(object sender , RoutedEventArgs e)
        {
            var btn = sender as Button;
            var vm = btn.DataContext as Scene;
            vm.AddGameEntityCommand.Execute(new GameEntity(vm) { Name = "Empty Game Entity" });
        }

        private void OnGameEntities_ListBox_SelectionChanged(object sender,SelectionChangedEventArgs e)
        {
            var listbox = sender as ListBox;           
            var newSelection = listbox.SelectedItems.Cast<GameEntity>().ToList();
            var previousSelection = newSelection.Except(e.AddedItems.Cast<GameEntity>()).Concat(e.RemovedItems.Cast<GameEntity>()).ToList();

            Project.UndoRedo.Add(new UndoRedoAction(
                () =>//undo action
                {
                    listbox.UnselectAll();
                    previousSelection.ForEach(x => (listbox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                },
                () => 
                {
                    listbox.UnselectAll();
                    newSelection.ForEach(x => (listbox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                },//redo action
                "Selection Changed"
                ));
            MSGameEntity msEntity = null;
            if (newSelection.Any())
            {
                msEntity = new MSGameEntity(newSelection);
            }
            GameEntityView.Instance.DataContext = msEntity;
        }
    }
}
