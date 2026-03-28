import re
from collections import deque, namedtuple

# --- 配置与数据结构 ---
REG_32 = ["%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi"]
REG_64 = ["%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi"]

# 题目允许的 NOP 指令 (单字节或双字节组合)
SAFE_BYTES = {
    "90",
    "20c0",
    "20c9",
    "20d2",
    "20db",
    "08c0",
    "08c9",
    "08d2",
    "08db",
    "38c0",
    "38c9",
    "38d2",
    "38db",
    "84c0",
    "84c9",
    "84d2",
    "84db",
}

Gadget = namedtuple("Gadget", ["addr", "type", "src", "dst", "raw", "block"])


class ROPFinder:
    def __init__(self, dump_text):
        self.bytes_map = []
        self.addr_to_block = {}
        self._parse(dump_text)

    def _parse(self, text):
        """解析汇编文本，建立字节映射和函数块映射"""
        blocks = re.split(r"\n(?=[0-9a-f]+ <.*>:)", text.strip())
        for block in blocks:
            lines = block.strip().split("\n")
            if not lines:
                continue
            for line in lines:
                match = re.match(r"\s*([0-9a-f]+):\s+(([0-9a-f]{2}\s)+)", line)
                if match:
                    base_addr = int(match.group(1), 16)
                    hex_bytes = match.group(2).strip().split()
                    for i, b in enumerate(hex_bytes):
                        addr = base_addr + i
                        self.bytes_map.append((addr, b))
                        self.addr_to_block[addr] = block

    def _is_safe(self, seq):
        """检查指令与 ret 之间的填充字节是否安全"""
        i = 0
        while i < len(seq):
            if seq[i] == "90":
                i += 1
            elif i + 1 < len(seq) and (seq[i] + seq[i + 1]) in SAFE_BYTES:
                i += 2
            else:
                return False
        return True

    def find_all_gadgets(self):
        """扫描所有潜在的 popq 和 movl gadgets"""
        found = {}
        # 遍历字节流寻找 ret (c3)
        for i, (addr, b) in enumerate(self.bytes_map):
            if b != "c3":
                continue

            # 从 c3 往回看最多 10 个字节
            for j in range(1, 10):
                if i - j < 0:
                    break
                start_idx = i - j
                start_addr, op = self.bytes_map[start_idx]
                seq = [x[1] for x in self.bytes_map[start_idx : i + 1]]

                res = None
                # 识别 popq (58-5f)
                val = int(op, 16)
                if 0x58 <= val <= 0x5F and self._is_safe(seq[1:-1]):
                    res = ("popq", None, REG_64[val - 0x58])

                # 识别 movl (89 c0-ff)
                elif op == "89" and len(seq) >= 3:
                    modrm = int(seq[1], 16)
                    if 0xC0 <= modrm <= 0xFF and self._is_safe(seq[2:-1]):
                        res = (
                            "movl",
                            REG_32[(modrm - 0xC0) // 8],
                            REG_32[(modrm - 0xC0) % 8],
                        )

                if res:
                    g = Gadget(
                        hex(start_addr),
                        res[0],
                        res[1],
                        res[2],
                        " ".join(seq),
                        self.addr_to_block[start_addr],
                    )
                    key = (g.addr, g.type)
                    # 保留最短的 gadget
                    if key not in found or len(seq) < len(found[key].raw.split()):
                        found[key] = g
        return list(found.values())

    @staticmethod
    def search_paths(gadgets, target="%esi"):
        """BFS 寻找从任意 popq 到目标寄存器的路径"""
        results = []
        pops = [g for g in gadgets if g.type == "popq"]
        movs = [g for g in gadgets if g.type == "movl"]

        # 队列存储: (当前寄存器, [路径步骤])
        queue = deque([(p.dst.replace("%r", "%e"), [p]) for p in pops])

        for _ in range(6):  # 限制跳数
            for _ in range(len(queue)):
                curr_reg, path = queue.popleft()
                if curr_reg == target:
                    results.append(path)
                    continue
                for m in movs:
                    if m.src == curr_reg and m not in path:
                        queue.append((m.dst, path + [m]))
        return results


if __name__ == "__main__":

    farm_dump = """
    0000000000401994 <start_farm>:
      401994:	b8 01 00 00 00       	mov    $0x1,%eax
      401999:	c3                   	ret

    000000000040199a <getval_142>:
      40199a:	b8 fb 78 90 90       	mov    $0x909078fb,%eax
      40199f:	c3                   	ret

    00000000004019a0 <addval_273>:
      4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
      4019a6:	c3                   	ret

    00000000004019a7 <addval_219>:
      4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
      4019ad:	c3                   	ret

    00000000004019ae <setval_237>:
      4019ae:	c7 07 48 89 c7 c7    	movl   $0xc7c78948,(%rdi)
      4019b4:	c3                   	ret

    00000000004019b5 <setval_424>:
      4019b5:	c7 07 54 c2 58 92    	movl   $0x9258c254,(%rdi)
      4019bb:	c3                   	ret

    00000000004019bc <setval_470>:
      4019bc:	c7 07 63 48 8d c7    	movl   $0xc78d4863,(%rdi)
      4019c2:	c3                   	ret

    00000000004019c3 <setval_426>:
      4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
      4019c9:	c3                   	ret

    00000000004019ca <getval_280>:
      4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
      4019cf:	c3                   	ret

    00000000004019d0 <mid_farm>:
      4019d0:	b8 01 00 00 00       	mov    $0x1,%eax
      4019d5:	c3                   	ret

    00000000004019d6 <add_xy>:
      4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
      4019da:	c3                   	ret

    00000000004019db <getval_481>:
      4019db:	b8 5c 89 c2 90       	mov    $0x90c2895c,%eax
      4019e0:	c3                   	ret

    00000000004019e1 <setval_296>:
      4019e1:	c7 07 99 d1 90 90    	movl   $0x9090d199,(%rdi)
      4019e7:	c3                   	ret

    00000000004019e8 <addval_113>:
      4019e8:	8d 87 89 ce 78 c9    	lea    -0x36873177(%rdi),%eax
      4019ee:	c3                   	ret

    00000000004019ef <addval_490>:
      4019ef:	8d 87 8d d1 20 db    	lea    -0x24df2e73(%rdi),%eax
      4019f5:	c3                   	ret

    00000000004019f6 <getval_226>:
      4019f6:	b8 89 d1 48 c0       	mov    $0xc048d189,%eax
      4019fb:	c3                   	ret

    00000000004019fc <setval_384>:
      4019fc:	c7 07 81 d1 84 c0    	movl   $0xc084d181,(%rdi)
      401a02:	c3                   	ret

    0000000000401a03 <addval_190>:
      401a03:	8d 87 41 48 89 e0    	lea    -0x1f76b7bf(%rdi),%eax
      401a09:	c3                   	ret

    0000000000401a0a <setval_276>:
      401a0a:	c7 07 88 c2 08 c9    	movl   $0xc908c288,(%rdi)
      401a10:	c3                   	ret

    0000000000401a11 <addval_436>:
      401a11:	8d 87 89 ce 90 90    	lea    -0x6f6f3177(%rdi),%eax
      401a17:	c3                   	ret

    0000000000401a18 <getval_345>:
      401a18:	b8 48 89 e0 c1       	mov    $0xc1e08948,%eax
      401a1d:	c3                   	ret

    0000000000401a1e <addval_479>:
      401a1e:	8d 87 89 c2 00 c9    	lea    -0x36ff3d77(%rdi),%eax
      401a24:	c3                   	ret

    0000000000401a25 <addval_187>:
      401a25:	8d 87 89 ce 38 c0    	lea    -0x3fc73177(%rdi),%eax
      401a2b:	c3                   	ret

    0000000000401a2c <setval_248>:
      401a2c:	c7 07 81 ce 08 db    	movl   $0xdb08ce81,(%rdi)
      401a32:	c3                   	ret

    0000000000401a33 <getval_159>:
      401a33:	b8 89 d1 38 c9       	mov    $0xc938d189,%eax
      401a38:	c3                   	ret

    0000000000401a39 <addval_110>:
      401a39:	8d 87 c8 89 e0 c3    	lea    -0x3c1f7638(%rdi),%eax
      401a3f:	c3                   	ret

    0000000000401a40 <addval_487>:
      401a40:	8d 87 89 c2 84 c0    	lea    -0x3f7b3d77(%rdi),%eax
      401a46:	c3                   	ret

    0000000000401a47 <addval_201>:
      401a47:	8d 87 48 89 e0 c7    	lea    -0x381f76b8(%rdi),%eax
      401a4d:	c3                   	ret

    0000000000401a4e <getval_272>:
      401a4e:	b8 99 d1 08 d2       	mov    $0xd208d199,%eax
      401a53:	c3                   	ret

    0000000000401a54 <getval_155>:
      401a54:	b8 89 c2 c4 c9       	mov    $0xc9c4c289,%eax
      401a59:	c3                   	ret

    0000000000401a5a <setval_299>:
      401a5a:	c7 07 48 89 e0 91    	movl   $0x91e08948,(%rdi)
      401a60:	c3                   	ret

    0000000000401a61 <addval_404>:
      401a61:	8d 87 89 ce 92 c3    	lea    -0x3c6d3177(%rdi),%eax
      401a67:	c3                   	ret

    0000000000401a68 <getval_311>:
      401a68:	b8 89 d1 08 db       	mov    $0xdb08d189,%eax
      401a6d:	c3                   	ret

    0000000000401a6e <setval_167>:
      401a6e:	c7 07 89 d1 91 c3    	movl   $0xc391d189,(%rdi)
      401a74:	c3                   	ret

    0000000000401a75 <setval_328>:
      401a75:	c7 07 81 c2 38 d2    	movl   $0xd238c281,(%rdi)
      401a7b:	c3                   	ret

    0000000000401a7c <setval_450>:
      401a7c:	c7 07 09 ce 08 c9    	movl   $0xc908ce09,(%rdi)
      401a82:	c3                   	ret

    0000000000401a83 <addval_358>:
      401a83:	8d 87 08 89 e0 90    	lea    -0x6f1f76f8(%rdi),%eax
      401a89:	c3                   	ret

    0000000000401a8a <addval_124>:
      401a8a:	8d 87 89 c2 c7 3c    	lea    0x3cc7c289(%rdi),%eax
      401a90:	c3                   	ret

    0000000000401a91 <getval_169>:
      401a91:	b8 88 ce 20 c0       	mov    $0xc020ce88,%eax
      401a96:	c3                   	ret

    0000000000401a97 <setval_181>:
      401a97:	c7 07 48 89 e0 c2    	movl   $0xc2e08948,(%rdi)
      401a9d:	c3                   	ret

    0000000000401a9e <addval_184>:
      401a9e:	8d 87 89 c2 60 d2    	lea    -0x2d9f3d77(%rdi),%eax
      401aa4:	c3                   	ret

    0000000000401aa5 <getval_472>:
      401aa5:	b8 8d ce 20 d2       	mov    $0xd220ce8d,%eax
      401aaa:	c3                   	ret

    0000000000401aab <setval_350>:
      401aab:	c7 07 48 89 e0 90    	movl   $0x90e08948,(%rdi)
      401ab1:	c3                   	ret

    0000000000401ab2 <end_farm>:
      401ab2:	b8 01 00 00 00       	mov    $0x1,%eax
      401ab7:	c3                   	ret
      401ab8:	90                   	nop
      401ab9:	90                   	nop
      401aba:	90                   	nop
      401abb:	90                   	nop
      401abc:	90                   	nop
      401abd:	90                   	nop
      401abe:	90                   	nop
      401abf:	90                   	nop
    """

    finder = ROPFinder(farm_dump)
    all_gadgets = finder.find_all_gadgets()
    paths = finder.search_paths(all_gadgets, "%esi")

    print(f"找到 {len(paths)} 条纯净路径。")
    for i, path in enumerate(paths):
        print(f"\n路径 {i+1}: " + " -> ".join(f"{g.addr}" for g in path))
        for step in path:
            action = f"{step.type} {step.src if step.src else ''} -> {step.dst}"
            print(f"  [{step.addr}] {step.raw:<12} | {action}")
