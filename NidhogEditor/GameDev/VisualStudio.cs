using NidhogEditor.GameProject;
using NidhogEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading.Tasks;
using EnvDTE80;
using System.Threading;

namespace NidhogEditor.GameDev
{
    public enum BuildConfiguration
    {
        Debug,
        DebugEditor,
        Release,
        ReleaseEditor,
    }
    static class VisualStudio
    {
        private static readonly ManualResetEventSlim _resetEvent = new ManualResetEventSlim(false);
        private static readonly string _progID = "VisualStudio.DTE.16.0";
        private static readonly object _lock = new object();
        private static readonly string[] _buildConfigurationNames = new string[] { "Debug", "DebugEditor", "Release", "ReleaseEditor" };
        private static EnvDTE80.DTE2 _vsInstance = null;


        public static bool BuildSucceeded { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        public static string GetConfigurationName(BuildConfiguration config) => _buildConfigurationNames[(int)config];

        //binding context
        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);

        private static void CallOnSTAThread(Action action)
        {
            Debug.Assert(action != null);
            var thread = new Thread(() =>
            {
                MessageFilter.Register();
                try { action(); }
                catch (Exception ex) { Logger.Log(MessageType.Warning, ex.Message); }
                finally { MessageFilter.Revoke(); }
            });

            thread.SetApartmentState(ApartmentState.STA);
            thread.Start();
            thread.Join();
        }

        public static void OpenVisualStudio_Internal(string solutionPath)
        {
            IRunningObjectTable rot = null;
            IEnumMoniker monikerTable = null;
            IBindCtx bindCtx = null;
            try
            {
                if (_vsInstance == null)
                {
                    // 找到并打开vs

                    //返回指向本地运行对象表上的 IRunningObjectTable 接口的指针
                    //获取访问运行对象表的权限
                    var hResult = GetRunningObjectTable(0, out rot);
                    //如果小于0则返回异常
                    if (hResult < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HRESULT: {hResult:X8}");

                    //为我们提供一个可枚举的表
                    rot.EnumRunning(out monikerTable);
                    //先对表进行重置
                    monikerTable.Reset();

                    //同样的对其是否可用进行检查
                    hResult = CreateBindCtx(0, out bindCtx);
                    if (hResult < 0 || bindCtx == null) throw new COMException($"CreateBindCtx() returned HRESULT: {hResult:X8}");

                    //遍历表
                    IMoniker[] currentMoniker = new IMoniker[1];
                    while (monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        string name = string.Empty;
                        //获取进程名
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (name.Contains(_progID))
                        {
                            hResult = rot.GetObject(currentMoniker[0], out object obj);
                            if (hResult < 0 || obj == null) throw new COMException($"Running object table's GetObject() returned HRESULT: {hResult:X8}");

                            //通过这个转化到instance
                            EnvDTE80.DTE2 dte = obj as EnvDTE80.DTE2;
                            var solutionName = string.Empty;
                            CallOnSTAThread(() =>
                            {
                                solutionName = dte.Solution.FullName;
                            });

                            if (solutionName == solutionPath)
                            {
                                _vsInstance = dte;
                                break;
                            }
                        }
                    }
                    //从硬盘启用vs
                    if (_vsInstance == null)
                    {
                        Type visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        //创建上面这种Type的instance
                        _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "failed to open Visual Studio");
            }
            finally
            {
                //资源释放
                if (monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }
        }
        public static void OpenVisualStudio(string solutionPath)
        {
            lock (_lock) { OpenVisualStudio_Internal(solutionPath); }
        }

        private static void CloseVisualStudio_Internal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance?.Solution.IsOpen == true)
                {
                    _vsInstance.ExecuteCommand("File.SaveAll");
                    _vsInstance.Solution.Close(true);
                }
                _vsInstance?.Quit();
                _vsInstance = null;
            });
        }

        public static void CloseVisualStudio()
        {
            lock (_lock) { CloseVisualStudio_Internal(); }
        }


        private static bool AddFilesToSolution_Internal(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenVisualStudio_Internal(solution);
            try
            {
                if (_vsInstance != null)
                {
                    CallOnSTAThread(() =>
                    {
                        if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(solution);
                        else _vsInstance.ExecuteCommand("File.SaveAll");

                        foreach (EnvDTE.Project project in _vsInstance.Solution.Projects)
                        {
                            if (project.UniqueName.Contains(projectName))
                            {
                                foreach (var file in files)
                                {
                                    project.ProjectItems.AddFromFile(file);
                                }
                            }
                        }

                        var cpp = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                        if (!string.IsNullOrEmpty(cpp))
                        {
                            _vsInstance.ItemOperations.OpenFile(cpp, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                        }
                        _vsInstance.MainWindow.Activate();
                        _vsInstance.MainWindow.Visible = true;
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine("failed to add files to Visual Studio project");
                return false;
            }
            return true;
        }

        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            lock (_lock) { return AddFilesToSolution_Internal(solution, projectName, files); }
        }

        private static void OnBuildSolutionBegin(string project, string projectConfig, string platform, string solutionConfig)
        {
            if (BuildDone) return;
            Logger.Log(MessageType.Info, $"Building {project}, {projectConfig}, {platform}, {solutionConfig}");
        }

        private static void OnBuildSolutionDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            if (BuildDone) return;

            if (success) Logger.Log(MessageType.Info, $"Building {projectConfig} configuration succeeded");
            else Logger.Log(MessageType.Error, $"Building {projectConfig} configuration failed");

            BuildDone = true;
            BuildSucceeded = success;
            _resetEvent.Set();
        }

        private static bool IsDebugging_Internal()
        {
            bool result = false;
            CallOnSTAThread(() =>
            {
                result = _vsInstance != null &&
                    (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
            });

            return result;
        }

        public static bool IsDebugging()
        {
            lock (_lock) { return IsDebugging_Internal(); }
        }

        private static void BuildSolution_Internal(Project project, BuildConfiguration buildConfig, bool showWindow)
        {

            if (IsDebugging_Internal())
            {
                Logger.Log(MessageType.Error, "Visual Studio is currenty running a process.");
                return;
            }

            OpenVisualStudio_Internal(project.Solution);
            BuildDone = BuildSucceeded = false;

            CallOnSTAThread(() =>
            {
                if (!_vsInstance.Solution.IsOpen)
                    _vsInstance.Solution.Open(project.Solution);
            });

            _vsInstance.MainWindow.Visible = showWindow;
            _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildSolutionBegin;
            _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildSolutionDone;

            var configName = GetConfigurationName(buildConfig);

            try
            {
                foreach (var pdbFile in Directory.GetFiles(Path.Combine($"{project.Path}", $@"x64\{configName}"), "*.pdb"))
                {
                    File.Delete(pdbFile);
                }
            }
            catch (Exception ex) { Debug.WriteLine(ex.Message); }

            CallOnSTAThread(() =>
            {
                _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                _vsInstance.ExecuteCommand("Build.BuildSolution");
                _resetEvent.Wait();
                _resetEvent.Reset();
            });
        }

        public static void BuildSolution(Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            lock (_lock) { BuildSolution_Internal(project, buildConfig, showWindow); }
        }



        //控制游戏运行
        private static void Run_Internal(Project project, BuildConfiguration buildConfig, bool debug)
        {
            CallOnSTAThread(() =>
            {
                //确定是否在调试状态下启动exe
                if (_vsInstance != null && !IsDebugging_Internal() && BuildSucceeded)
                {
                    _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
                }
            });
        }

        public static void Run(Project project, BuildConfiguration buildConfig, bool debug)
        {
            lock (_lock) { Run_Internal(project, buildConfig, debug); }
        }

        private static void Stop_Internal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && IsDebugging_Internal())
                {
                    _vsInstance.ExecuteCommand("Debug.StopDebugging");
                }
            });
        }
        //控制游戏停止
        public static void Stop()
        {
            lock (_lock) { Stop_Internal(); }
        }
    }


    // 包含 IOleMEssageFilter 线程错误处理函数的类
    public class MessageFilter : IMessageFilter
    {
        private const int SERVERCALL_ISHANDLED = 0;
        private const int PENDINGMSG_WAITDEFPROCESS = 2;
        private const int SERVERCALL_RETRYLATER = 2;

        [DllImport("Ole32.dll")]
        private static extern int CoRegisterMessageFilter(IMessageFilter newFilter, out IMessageFilter oldFilter);

        public static void Register()
        {
            IMessageFilter newFilter = new MessageFilter();
            int hr = CoRegisterMessageFilter(newFilter, out var oldFilter);
            Debug.Assert(hr >= 0, "Registering COM IMessageFilter failed.");
        }

        public static void Revoke()
        {
            int hr = CoRegisterMessageFilter(null, out var oldFilter);
            Debug.Assert(hr >= 0, "Unregistering COM IMessageFilter failed.");
        }


        int IMessageFilter.HandleInComingCall(int dwCallType, System.IntPtr hTaskCaller, int dwTickCount, System.IntPtr lpInterfaceInfo)
        {
            //returns the flag SERVERCALL_ISHANDLED. 
            return SERVERCALL_ISHANDLED;
        }


        int IMessageFilter.RetryRejectedCall(System.IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
        {
            // Thread call was refused, try again. 
            if (dwRejectType == SERVERCALL_RETRYLATER)
            {
                // retry thread call at once, if return value >=0 & <100. 
                Debug.WriteLine("COM server busy. Retrying call to EnvDTE interface.");
                return 500;
            }
            // Too busy. Cancel call.
            return -1;
        }


        int IMessageFilter.MessagePending(System.IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
        {
            return PENDINGMSG_WAITDEFPROCESS;
        }
    }


    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"),
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMessageFilter
    {

        [PreserveSig]
        int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo);


        [PreserveSig]
        int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType);


        [PreserveSig]
        int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType);
    }
}

