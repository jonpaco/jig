.class Ljig/JigMainThreadRunner$1;
.super Ljava/lang/Object;
.implements Ljava/lang/Runnable;

.field private final fn:J
.field private final ctx:J

.method constructor <init>(JJ)V
    .locals 0
    .param p1, "fn"
    .param p3, "ctx"
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    iput-wide p1, p0, Ljig/JigMainThreadRunner$1;->fn:J
    iput-wide p3, p0, Ljig/JigMainThreadRunner$1;->ctx:J
    return-void
.end method

.method public run()V
    .locals 4
    iget-wide v0, p0, Ljig/JigMainThreadRunner$1;->fn:J
    iget-wide v2, p0, Ljig/JigMainThreadRunner$1;->ctx:J
    invoke-static {v0, v1, v2, v3}, Ljig/JigMainThreadRunner;->executePendingCallback(JJ)V
    return-void
.end method
