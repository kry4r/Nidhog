using System;
using System.Collections.Generic;
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
    }
    public class DelayEventTimerArgs : EventArgs
    {
        //是否重复事件
        public bool RepeatEvent { get; set; }
        public object Data { get; set; }

        public DelayEventTimerArgs(object data)
        {
            Data = data;
        }
    }

    //延迟事件计时器，在调用事件handle之前延迟一段时间
    public class DelayEventTimer
    {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private DateTime _lastEventTime = DateTime.Now;
        private object _data;

        public event EventHandler<DelayEventTimerArgs> Triggered;

        public void Trigger(object data = null)
        {
            _data = data;
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
