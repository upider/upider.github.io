# 删除osd

### 查看osd
```bash
osd tree
```

### 停止osd进程
```bash
systemctl stop  ceph-osd@0
```

### 下线osd
```bash
ceph osd out 0
ceph osd rm 0
```

### crush中删除osd
```bash
ceph osd crush remove 0
```

### 删除磁盘
```bash
ceph auth del osd.0
```
