void *
memcpy (void *dest, const void *src, size_t len)
{
  volatile char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}

void *
memset (void *dest, int val, size_t len)
{
  volatile unsigned char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}
