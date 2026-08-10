# Quick Start

### Requirements

- C23 Clang
- SDL3 & SDL3_image

### Clone, Build, & Run

```bash
git clone https://github.com/jspingu/threefold.git
cd threefold
make -j$(nproc)
./out
```

### Vectorization Options

By default, vectorized function variants are built for all supported SIMD extensions on the target architecture and called through dynamic dispatch.

To build a tailored binary with static dispatch instead:

```bash
CFLAGS="-march=native" VECTORIZATION="static" make -j$(nproc)
```

Supported extensions:
- x86: SSE2, AVX2, AVX512F
- ARM: NEON (v7), SVE

# Showcase

<img width="1920" height="1200" alt="scene" src="https://github.com/user-attachments/assets/da71002f-86ac-46d6-ba63-f308e75f8652" />

Software-rendered scene — [Utah teapot](https://users.cs.utah.edu/~dejohnso/models/teapot.html) (22,855 triangles), skybox, lights, phong shading, cubemap reflections, gamma correction

1920x1200@~50FPS on my X1P42100
