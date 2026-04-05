.class public Ljig/JigMainThreadRunner;
.super Ljava/lang/Object;

.field private static final sMainHandler:Landroid/os/Handler;

.method static constructor <clinit>()V
    .locals 2
    new-instance v0, Landroid/os/Handler;
    invoke-static {}, Landroid/os/Looper;->getMainLooper()Landroid/os/Looper;
    move-result-object v1
    invoke-direct {v0, v1}, Landroid/os/Handler;-><init>(Landroid/os/Looper;)V
    sput-object v0, Ljig/JigMainThreadRunner;->sMainHandler:Landroid/os/Handler;
    return-void
.end method

.method public static postToMainThread(JJ)V
    .locals 3
    .param p0, "fnPtr"
    .param p2, "ctxPtr"

    sget-object v0, Ljig/JigMainThreadRunner;->sMainHandler:Landroid/os/Handler;
    new-instance v1, Ljig/JigMainThreadRunner$1;
    invoke-direct {v1, p0, p1, p2, p3}, Ljig/JigMainThreadRunner$1;-><init>(JJ)V
    invoke-virtual {v0, v1}, Landroid/os/Handler;->post(Ljava/lang/Runnable;)Z
    return-void
.end method

.method static native executePendingCallback(JJ)V
.end method
