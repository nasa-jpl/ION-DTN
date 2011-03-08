#ifndef _SHA1_H_
#define _SHA1_H_

typedef struct {
	int	something;
} moz_SHA_CTX;

void moz_SHA1_Init(moz_SHA_CTX *ctx);
void moz_SHA1_Update(moz_SHA_CTX *ctx, const void *dataIn, int len);
void moz_SHA1_Final(unsigned char hashout[20], moz_SHA_CTX *ctx);

#define git_SHA_CTX	moz_SHA_CTX
#define git_SHA1_Init	moz_SHA1_Init
#define git_SHA1_Update	moz_SHA1_Update
#define git_SHA1_Final	moz_SHA1_Final

#endif
