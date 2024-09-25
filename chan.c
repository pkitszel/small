/* a beginning of go-like channel for C11 */

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define atomic _Atomic
typedef atomic int spinlock;

void spin_lock(spinlock *lock)
{
	int expected;
	do {
		expected = 0;
	} while (!atomic_compare_exchange_weak(lock, &expected, 1));
}

void spin_unlock(spinlock *lock)
{
	*lock = 0;
}

struct chan {
	unsigned cap; /* how big the @buf is */
	unsigned put; /* index the next input will be put on */
	unsigned get; /* index the next output will be get from */
	spinlock lock; /* put & get update must be covered by a single
			* transaction, at least for @cap==1 case, but would
			* require atomics anyway for higher @cap */
	atomic unsigned done; /* readers will get false response from chan_get()
			       * when @done, all producers must signal that */
	atomic unsigned src_cnt; /* bumped once by each producer */
	int buf[]; /* TODO: support unbuffered case to (as an excercise) */
};

void chan_done(struct chan *ch)
{
	if (!--ch->src_cnt)
		ch->done = true;
}

void chan_put(struct chan *ch, int x)
{
	while (true) {
		spin_lock(&ch->lock);
		if (ch->put - ch->get < ch->cap)
			break;
		spin_unlock(&ch->lock);
	}

	ch->buf[ch->put++ % ch->cap] = x;
	spin_unlock(&ch->lock);
}

bool chan_get(struct chan *ch, int *x)
{
	while (true) {
		spin_lock(&ch->lock);
		if (ch->put != ch->get)
			break;
		spin_unlock(&ch->lock);

		if (ch->done)
			return false;
	}

	*x = ch->buf[ch->get++ % ch->cap];
	spin_unlock(&ch->lock);
	return true;
}

int emitter(void *priv)
{
	struct chan *ch = priv;

	ch->src_cnt++;
	for (int i = 0; i < 1000; ++i) {
		chan_put(ch, i);
	}

	chan_done(ch);

	return 0;
}

int consumer(void *priv)
{
	struct chan *ch = priv;

	for (int x; chan_get(ch, &x); ) {
		printf("%d\n", x);
	}

	return 0;
}

int main()
{
	const int buf_size = 7; /* any positive value will do */
	thrd_t t1, t2, t3, t4;
	struct chan *ch;
	int err;

	ch = calloc(1, sizeof(struct chan) + sizeof(int) * buf_size);
	ch->cap = buf_size;

	err = thrd_create(&t1, emitter, ch);
	assert(!err);
	err = thrd_create(&t2, consumer, ch);
	assert(!err);
	err = thrd_create(&t3, consumer, ch);
	assert(!err);
	err = thrd_create(&t4, emitter, ch);
	assert(!err);

	thrd_join(t1, &err);
	assert(!err);
	thrd_join(t2, &err);
	assert(!err);
	thrd_join(t3, &err);
	assert(!err);
	thrd_join(t4, &err);
	assert(!err);
	/* TODO: assert also thrd_join() retvals */
}
