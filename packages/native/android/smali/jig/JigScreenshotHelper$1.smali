.class Ljig/JigScreenshotHelper$1;
.super Ljava/lang/Object;
.implements Landroid/view/PixelCopy$OnPixelCopyFinishedListener;

.field private final result:[I
.field private final latch:Ljava/util/concurrent/CountDownLatch;

.method constructor <init>([ILjava/util/concurrent/CountDownLatch;)V
    .locals 0
    .param p1, "result"
    .param p2, "latch"
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    iput-object p1, p0, Ljig/JigScreenshotHelper$1;->result:[I
    iput-object p2, p0, Ljig/JigScreenshotHelper$1;->latch:Ljava/util/concurrent/CountDownLatch;
    return-void
.end method

.method public onPixelCopyFinished(I)V
    .locals 2
    .param p1, "copyResult"

    # result[0] = copyResult
    iget-object v0, p0, Ljig/JigScreenshotHelper$1;->result:[I
    const/4 v1, 0x0
    aput p1, v0, v1

    # latch.countDown()
    iget-object v0, p0, Ljig/JigScreenshotHelper$1;->latch:Ljava/util/concurrent/CountDownLatch;
    invoke-virtual {v0}, Ljava/util/concurrent/CountDownLatch;->countDown()V

    return-void
.end method
