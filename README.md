قدم ۰ — قراردادها را قفل کن (قبل از هر کد جدی)

هدف: از روز اول بدانیم “ورودی دقیقاً چیست و خروجی دقیقاً چیست”.

قرارداد ورودی (Eventها)
حداقل این‌ها را مشخص کن:

آیا پیام ورودی در UDS برای هر vtime جدا می‌آید یا ممکن است چند vtime در یک پیام بیاید؟

framing پیام در UDS:

اگر SOCK_STREAM است: پیام‌ها line-delimited هستند؟ یا length-prefixed؟

اگر SOCK_DGRAM است: هر datagram یک JSON کامل است.

اسکیمای JSON: فیلدهای مشترک (type, task_id, vtime, value/payload …)

قرارداد خروجی
README می‌گوید خروجی حداقل این فرم را بده: {"vtime": 10, "schedule": ["T1", "idle", "T2"]} 
GitHub

پس خروجی را از روز اول “line-oriented JSON” چاپ کن (هر tick یک خط) که تست‌گیر راحت بخواند.

نکته: اگر اسکیمای دقیق را در README یا docs/PROTOCOL.md بنویسی، هم خودت هم استاد خیلی راحت‌تر همه چیز را می‌فهمید.

قدم ۱ — اسکلت Build و Targetها (CMake)

هدف: خیلی زود بتوانی build && run کنی، حتی قبل از کامل شدن scheduler.

پیشنهاد عملی:

یک library بساز (مثلاً alfs_core) شامل Task/RunQueue/Scheduler/Socket/Json

یک executable بساز (مثلاً alfs) که فقط CLI و loop را هندل کند.

این باعث می‌شود هر بخش را جداگانه تست کنی.

قدم ۲ — جدول weight و مفهوم vruntime را دقیق پیاده کن

README می‌گوید weight را از جدول nice لینوکس بگیر و delta = quantum * (1024 / weight) 
GitHub

این دقیقاً همسو با منطق CFS است: nice=0 مرجع (NICE_0_LOAD=1024) و vruntime با وزن scale می‌شود. 
lkml.org
+2
sandipc-iitkgp.github.io
+2

کار عملی

یک آرایه/مپ ثابت بساز: nice [-20..19] -> weight

نمونه‌ی مقادیر معروف: برای nice=0 وزن 1024، nice=1 وزن 820، … 
lkml.org

تابع:

weight = nice_to_weight(nice)

delta_vruntime = quantum * (NICE_0_LOAD / weight) (نوع double یا fixed-point)

اگر استاد گیر داد “چرا unblock vruntime را max می‌کنی؟” می‌توانی به min_vruntime و فلسفه‌ی placement اشاره کنی. 
Kernel.org

قدم ۳ — مدل داده‌ها (Task/CPU/Cgroup) را نهایی کن

طبق README حداقل این‌ها را نگه دار: state, nice, weight, vruntime, affinity, cgroup_id, current_cpu. 
GitHub

پیشنهاد من برای اینکه بعداً به مشکل نخوری:

TaskState { RUNNABLE, BLOCKED, EXITED }

در Task یک bool in_runqueue داشته باش (کمک می‌کند دوباره دوباره push نکنی)

یک registry مرکزی: unordered_map<int, Task> برای پیدا کردن سریع task با id.

قدم ۴ — RunQueue واقعی: Min-Heap ولی با قابلیت remove

README می‌گوید RunQueue یک min-heap با کلید vruntime است. 
GitHub

چالش: std::priority_queue در C++ remove دلخواه ندارد.

سه راه استاندارد (به ترتیب توصیه):

هیپ سفارشی با vector<Task*> heap + unordered_map<task_id, index>

push/pop/remove همه O(log n)

بهترین و “قابل دفاع” برای پروژه زمان‌بندی

Lazy deletion

وقتی block/exit شد، یک flag بزن؛ موقع pop اگر invalid بود discard کن.

ساده‌تر، ولی باید حواست به رشد heap باشد.

استفاده از std::multiset

از نظر دیتااستراکچر “RBTree” می‌شود، ولی اگر تکلیف گفته “min-heap”، ممکن است گیر بدهند.

قدم ۵ — min_vruntime و max_vruntime را درست نگه دار

این دو تا، کلید رفتارهای create/unblock/yield هستند (README هم استفاده کرده). 
GitHub

min_vruntime: کوچک‌ترین vruntime بین runnableها (در عمل top heap)
در طراحی CFS هم min_vruntime معیار مهمی برای قرار دادن task تازه-فعال است. 
Kernel.org

max_vruntime: بزرگ‌ترین vruntime بین runnableها
برای پیاده‌سازی ساده‌ی TASK_YIELD (بردن ته صف) خوب است. 
GitHub

قدم ۶ — ترتیب اعمال eventها (قفل طلایی پروژه)

README می‌گوید در هر vtime این ترتیب:
receive events → apply events → schedule CPUs → output 
GitHub

این را دقیقاً همین‌طور نگه دار تا “زمان” قاطی نشود.

قدم ۷ — پیاده‌سازی eventها، ولی با نکات ریزِ درست

README ترتیب eventها را داده؛ من فقط نکات edge-case را اضافه می‌کنم: 
GitHub

TASK_CREATE

اگر task_id تکراری بود: یا ignore کن یا update؛ ولی crash نکن.

vruntime = min_vruntime (همان چیزی که README گفته) 
GitHub
+1

state=RUNNABLE و push

TASK_BLOCK

اگر task روی CPU در حال اجراست: current_cpu = None و CPU آزاد

از runqueue remove (یا lazy invalidate)

state=BLOCKED

TASK_UNBLOCK

state=RUNNABLE

vruntime = max(old_vruntime, min_vruntime) (README) 
GitHub
+1

push

TASK_EXIT

از همه جا پاک کن: registry + runqueue + cpu.current اگر روی CPU بود

state=EXITED

TASK_YIELD

اگر RUNNABLE است: vruntime = max_vruntime و دوباره push (README) 
GitHub

(هدف: عقب بیفتد.)

TASK_SETNICE

nice جدید را clamp کن به [-20..19]

weight را از جدول دوباره محاسبه کن 
lkml.org

(اختیاری) برای عدالت: اگر تغییر nice شدید است، vruntime را دست نزن (ساده) یا یک normalize کوچک بده (پیچیده؛ لازم نیست)

TASK_SET_AFFINITY

cpu_affinity set را آپدیت کن (README) 
GitHub

اگر task الان روی CPUای است که دیگر مجاز نیست:

همان tick preemptش کن (از CPU بردار و برگردان runqueue)

قدم ۸ — خود الگوریتم Scheduling در هر tick (چند CPU)

README نسخه‌ی ساده را گفته: برای هر CPU کوچک‌ترین vruntime، اگر affinity نخورد رد کن، اگر نبود idle؛ بعد vruntime را آپدیت کن و دوباره push. 
GitHub

من این را کمی “اجرایی‌تر” می‌کنم که duplicate روی چند CPU نشود:

الگوریتم پیشنهادی tick(vtime):

یک vector<Task*> picked;

برای cpu=0..N-1:

از heap pop کن تا یک task پیدا شود که:

RUNNABLE باشد

affinity اجازه بدهد

روی CPU دیگری در همین tick انتخاب نشده باشد

اگر پیدا نشد → schedule[cpu] = idle

اگر پیدا شد:

schedule[cpu] = task_id

task را داخل picked نگه دار (فعلاً push نکن)

بعد از اینکه انتخاب‌ها تمام شد:

برای هر task در picked:

task.vruntime += quantum * (NICE_0_LOAD / weight) 
lkml.org
+2
sandipc-iitkgp.github.io
+2

push به heap

min_vruntime را از top heap آپدیت کن.

قدم ۹ — preemption را درست حساب کن

README می‌گوید: هر tick همه CPUها از صفر schedule شوند و اگر task قبلی != جدید → preemption++ 
GitHub

نکته‌های عملی:

اگر قبلی idle بوده و جدید task شده، این را هم “preempt” حساب می‌کنی یا نه؟
برای سادگی: حساب نکن (چون preempt معمولاً قطع یک کارِ درحال اجراست).

ولی اگر استاد/تست‌گیر دقیق بود: همان تعریف README را literal اجرا کن.

قدم ۱۰ — UDS: پیاده‌سازی مقاوم و ساده

README دو جا اشاره کرده: “ساخت UDS و دریافت JSON” 
GitHub
 و در اسکلت main هم ایده‌ی loop آمده. 
GitHub

پیشنهاد من برای کم ریسک‌ترین حالت:

Unix Domain Stream:

socket(AF_UNIX, SOCK_STREAM, 0)

bind(path), listen, accept

خواندن line-by-line (هر خط یک JSON)

اگر تست‌گیر datagram بود:

SOCK_DGRAM و هر recv یک JSON

نکته‌ی حیاتی: قبل از bind اگر فایل socket وجود دارد، unlink(path) کن (وگرنه bind fail می‌شود).

قدم ۱۱ — تست‌های سریع ولی کافی (برای اینکه گیر نکنی)

README هم پیشنهاد “تست دستی” داده. 
GitHub

من یک چک‌لیست تست حداقلی می‌دهم:

۲ تسک، ۱ CPU، nice برابر

باید تقریباً round-robin-like شود (به خاطر vruntime برابر)

۲ تسک، ۱ CPU، nice متفاوت

آن که nice کمتر (وزن بیشتر) دارد باید سهم بیشتری بگیرد (vruntime کندتر رشد می‌کند). 
lkml.org
+1

block/unblock

تسک blocked نباید schedule شود

بعد از unblock با max(vruntime, min_vruntime) وارد شود (نه اینکه unfair جلو بیفتد). 
Kernel.org

affinity

تسکی که فقط روی CPU 0 مجاز است، نباید روی CPU 1 بیاید.