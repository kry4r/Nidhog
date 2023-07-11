using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace NidhogEditor.Utilities
{
    public static class ID
    {
        public static int INVALID_ID => -1;
        public static bool IsValid(int id) => id != INVALID_ID;
    }
    public static class MathUtil
    {
        public static float Epsilon => 0.00001f;

        public static bool IsTheSameAs(this float value, float other)
        {
            return Math.Abs(value - other) < Epsilon;
        }

        public static bool IsTheSameAs(this float? value, float? other)
        {
            if (!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }

        // Align by rounding up. Will result in a multiple of 'alignment' that is greater than or equal to 'size'.
        public static long AlignSizeUp(long size, long alignment)
        {
            Debug.Assert(alignment > 0, "Alignment must be non-zero.");
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment should be a power of 2.");
            return ((size + mask) & ~mask);
        }

        // Align by rounding down. Will result in a multiple of 'alignment' that is less than or equal to 'size'.
        public static long AlignSizeDown(long size, long alignment)
        {
            Debug.Assert(alignment > 0, "Alignment must be non-zero.");
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment should be a power of 2.");
            return (size & ~mask);
        }
    }
    public class DelayEventTimerArgs : EventArgs
    {
        //是否重复事件
        public bool RepeatEvent { get; set; }
        public IEnumerable<object> Data { get; set; }

        public DelayEventTimerArgs(IEnumerable<object> data)
        {
            Data = data;
        }
    }

    //延迟事件计时器，在调用事件handle之前延迟一段时间
    public class DelayEventTimer
    {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private readonly List<object> _data = new List<object>();
        private DateTime _lastEventTime = DateTime.Now;

        public event EventHandler<DelayEventTimerArgs> Triggered;

        public void Trigger(object data = null)
        {
            if (data != null)
            {
                _data.Add(data);
            }
            _lastEventTime = DateTime.Now;
            _timer.IsEnabled = true;
        }

        public void Disable()
        {
            _timer.IsEnabled = false;
        }

        private void OnTimerTick(object sender, EventArgs e)
        {
            if ((DateTime.Now - _lastEventTime) < _delay) return;
            var eventArgs = new DelayEventTimerArgs(_data);
            Triggered?.Invoke(this, eventArgs);
            if (!eventArgs.RepeatEvent)
            {
                _data.Clear();
            }
            _timer.IsEnabled = eventArgs.RepeatEvent;
        }

        //给一个delay，让我们在调用前等待
        public DelayEventTimer(TimeSpan delay, DispatcherPriority priority = DispatcherPriority.Normal)
        {
            _delay = delay;
            _timer = new DispatcherTimer(priority)
            {
                //防止过多时间等待
                //将这个调度计时器间隔设置为half of delay
                Interval = TimeSpan.FromMilliseconds(delay.TotalMilliseconds * 0.5)
            };
            _timer.Tick += OnTimerTick;

        }
    }
}
