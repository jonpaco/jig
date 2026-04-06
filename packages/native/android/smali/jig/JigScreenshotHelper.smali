.class public Ljig/JigScreenshotHelper;
.super Ljava/lang/Object;

# public static int capture(Window window, Bitmap bitmap)
.method public static capture(Landroid/view/Window;Landroid/graphics/Bitmap;)I
    .locals 7
    .param p0, "window"
    .param p1, "bitmap"

    # HandlerThread thread = new HandlerThread("jig-screenshot")
    new-instance v0, Landroid/os/HandlerThread;
    const-string v1, "jig-screenshot"
    invoke-direct {v0, v1}, Landroid/os/HandlerThread;-><init>(Ljava/lang/String;)V
    invoke-virtual {v0}, Landroid/os/HandlerThread;->start()V

    # CountDownLatch latch = new CountDownLatch(1)
    new-instance v1, Ljava/util/concurrent/CountDownLatch;
    const/4 v2, 0x1
    invoke-direct {v1, v2}, Ljava/util/concurrent/CountDownLatch;-><init>(I)V

    # int[] result = new int[]{-1}
    const/4 v2, 0x1
    new-array v2, v2, [I
    const/4 v3, 0x0
    const/4 v4, -0x1
    aput v4, v2, v3

    # listener = new JigScreenshotHelper$1(result, latch)
    new-instance v3, Ljig/JigScreenshotHelper$1;
    invoke-direct {v3, v2, v1}, Ljig/JigScreenshotHelper$1;-><init>([ILjava/util/concurrent/CountDownLatch;)V

    # handler = new Handler(thread.getLooper())
    new-instance v4, Landroid/os/Handler;
    invoke-virtual {v0}, Landroid/os/HandlerThread;->getLooper()Landroid/os/Looper;
    move-result-object v5
    invoke-direct {v4, v5}, Landroid/os/Handler;-><init>(Landroid/os/Looper;)V

    # PixelCopy.request(window, bitmap, listener, handler)
    invoke-static {p0, p1, v3, v4}, Landroid/view/PixelCopy;->request(Landroid/view/Window;Landroid/graphics/Bitmap;Landroid/view/PixelCopy$OnPixelCopyFinishedListener;Landroid/os/Handler;)V

    # try { latch.await(5, TimeUnit.SECONDS) }
    :try_start
    const-wide/16 v4, 0x5
    sget-object v6, Ljava/util/concurrent/TimeUnit;->SECONDS:Ljava/util/concurrent/TimeUnit;
    invoke-virtual {v1, v4, v5, v6}, Ljava/util/concurrent/CountDownLatch;->await(JLjava/util/concurrent/TimeUnit;)Z
    :try_end
    .catch Ljava/lang/InterruptedException; {:try_start .. :try_end} :catch

    :goto_finally
    # thread.quit()
    invoke-virtual {v0}, Landroid/os/HandlerThread;->quit()Z

    # return result[0]
    const/4 v3, 0x0
    aget v3, v2, v3
    return v3

    :catch
    move-exception v4
    goto :goto_finally
.end method
