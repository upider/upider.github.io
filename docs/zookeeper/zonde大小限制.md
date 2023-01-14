# zonde大小限制

znode默认大小限制1MB

## 修改方法

```yaml
configuration:
    #10MB
    jute.maxbufer=10485760
```

`jvmFlags: "-Djute.maxbuffer=10485760"`