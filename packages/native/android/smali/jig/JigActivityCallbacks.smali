.class public Ljig/JigActivityCallbacks;
.super Ljava/lang/Object;
.implements Landroid/app/Application$ActivityLifecycleCallbacks;

.method public constructor <init>()V
    .locals 0
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    return-void
.end method

.method public onActivityResumed(Landroid/app/Activity;)V
    .locals 0
    invoke-static {p1}, Ljig/JigActivityCallbacks;->nativeOnActivityResumed(Landroid/app/Activity;)V
    return-void
.end method

.method public onActivityPaused(Landroid/app/Activity;)V
    .locals 0
    invoke-static {p1}, Ljig/JigActivityCallbacks;->nativeOnActivityPaused(Landroid/app/Activity;)V
    return-void
.end method

.method public onActivityCreated(Landroid/app/Activity;Landroid/os/Bundle;)V
    .locals 0
    return-void
.end method

.method public onActivityStarted(Landroid/app/Activity;)V
    .locals 0
    return-void
.end method

.method public onActivityStopped(Landroid/app/Activity;)V
    .locals 0
    return-void
.end method

.method public onActivitySaveInstanceState(Landroid/app/Activity;Landroid/os/Bundle;)V
    .locals 0
    return-void
.end method

.method public onActivityDestroyed(Landroid/app/Activity;)V
    .locals 0
    return-void
.end method

.method private static native nativeOnActivityResumed(Landroid/app/Activity;)V
.end method

.method private static native nativeOnActivityPaused(Landroid/app/Activity;)V
.end method
