int isLessOrEqual(int x, int y) {
    // x <= y
  int smaller, sx, sy, same_sign, equal, mask, diff, diff_sign;
    sx = !(x >> 31); // 1 -> 1111 0 -> 0000
    sy = !(y >> 31);
    same_sign = !(sx ^ sy);

    equal = !(x ^ y);

    diff  = y + (~x + 1);

    diff_sign = !(diff >> 31);
    smaller = !(sx & ~sy) | (same_sign & diff_sign); 
    return equal | smaller;
}

int main() {
  isLessOrEqual(0x80000000, 0x7fffffff);
  return 0;
}