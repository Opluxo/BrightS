1.完善内核优化（优化对象：i5 1135G7 + 8GB RAM）
  - [x] 优化kmalloc：增加heap大小到8MB，实现真正的free功能
  - [x] 完善调度器：添加进程队列管理和轮转调度
2.完善kshell
  - [x] 添加命令历史功能（支持上下箭头键导航）
  - [x] 添加tab补全功能
3.根据File_Struct_Define修改目录结构
  - [x] 更新vfs.c以创建新的目录结构
  - [x] 更新kshell.c中的路径引用
  - [x] 更新kernel_main.c中的挂载路径
  - [x] 更新userinit.c中的系统配置路径
4.补全代码实现
  - [x] 优化runtime.c：添加64位优化的memcpy/memset/memmove/memcmp
  - [x] 完善sleep.c：添加sleep_us/sleep_ms/halt函数
  - [x] 完善clock.c：添加纳秒级时间支持（TSC）
  - [x] 扩展sysent.c：添加42个系统调用
  - [x] 完善paging.c：实现页表管理
  - [x] 完善proc.c：添加进程名、退出、父进程ID、僵尸进程状态
5.精简项目目录
  - [x] 删除v6fs相关文件
  - [x] 项目统一使用btrfs作为主文件系统
6.用户态补全
  - [x] ELF64加载器（elf.c/elf.h）
  - [x] 扩展proc结构支持用户态上下文（CR3、brk、kernel_stack、ctx）
  - [x] userinit支持加载ELF进入用户模式
  - [x] 系统调用全部改用VFS2
  - [x] 嵌入式shell ELF程序（支持help/echo/exit命令）
  - [x] sys_fork/sys_exec完善
7.中断与时钟
  - [x] PIC重映射（IRQ0-7→INT32-39, IRQ8-15→INT40-47）
  - [x] PIT定时器（100Hz周期中断）
  - [x] 时钟中断驱动的抢占式调度（10 tick量子）
  - [x] 键盘中断（IRQ1→INT33）
8.设备驱动
  - [x] PS/2键盘中断驱动输入缓冲（ring buffer）
  - [x] TTY行规程（cooked mode、echo、退格、tab）
  - [x] DevFS扩展（/dev/null, /dev/zero, /dev/rtc, /dev/kbd, /dev/disk0）
  - [x] 块设备接口扩展（name、type、total_blocks、block_size、ready、注册表）
  - [x] 管道IPC（pipe.c ring buffer + VFS集成）
9.文件系统
  - [x] VFS2抽象层（mount、resolve、open/close/read/write/stat/readdir）
  - [x] RAMFS VFS适配器
  - [x] DevFS VFS适配器
  - [x] Btrfs btree split/merge/insert_node/delete_leaf/search
