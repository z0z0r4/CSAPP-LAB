int logicalNeg(int x) {
    int a = x | (~x + 1); // get sign bit of x and -x
    int b = (a >> 31);
    int c = b + 1; // if x is 0, b is 0, c is 1; otherwise, b is -1, c is 0
    return c;
}

int main() {
  logicalNeg(0x80000000);
  return 0;
}