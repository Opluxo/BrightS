# BrightS Operating System Kernel / BrightS オペレーティングシステムカーネル

## 项目状态 / プロジェクト状態 / Project Status
✅ 内核正常引导 / カーネルが正常に起動 / Kernel boots successfully
✅ 支持 UEFI 启动 / UEFI起動をサポート / UEFI boot support
✅ x86_64 架构 / x86_64アーキテクチャ / x86_64 architecture
✅ 虚拟内存管理 / 仮想メモリ管理 / Virtual memory management
✅ 进程调度 / プロセススケジューリング / Process scheduling
✅ 终端 Shell / ターミナルシェル / Terminal Shell
✅ PS/2 键盘驱动 / PS/2キーボードドライバ / PS/2 keyboard driver
✅ AHCI 硬盘驱动 / AHCIハードディスクドライバ / AHCI hard drive driver
✅ NVMe 支持 / NVMeサポート / NVMe support
✅ TCP/IP 网络栈 / TCP/IPネットワークスタック / TCP/IP network stack

## 编译构建 / ビルド / Building
```bash
mkdir build && cd build
cmake ..
make -j8
```

## 运行测试 / テスト実行 / Running Tests
```bash
./scripts/run-qemu.sh
```

## 文档 / ドキュメント / Documentation

- **[English](docs/README.md)** - Full documentation
- **[日本語](docs/README_ja.md)** - 完全なドキュメント
- **[中文](docs/README_zh_CN.md)** - 完整文档

## 用户指南 / ユーザーガイド / User Guide
- [Command Reference (EN)](docs/user-guide/COMMAND_REFERENCE.md)
- [コマンドリファレンス (JA)](docs/user-guide/COMMAND_REFERENCE_ja.md)
- [命令参考手册 (CN)](docs/user-guide/COMMAND_REFERENCE_CN.md)

## 开发者指南 / 開発者ガイド / Developer Guide
- [Developer Guide (EN)](docs/developer-guide/README.md)
- [開発者ガイド (JA)](docs/developer-guide/README_ja.md)
- [开发者指南 (CN)](docs/developer-guide/README_zh_CN.md)
