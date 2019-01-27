# lua 5.3 modules 编写注意事项:

lua 5.3 需要编译成.so文件，与主程序共用同一个.so动态链接库。
编写方式为 在lua 5.3目录下，首先修改Makefile文件：

```
TO_LIB = liblua.a liblua.so

再修改src/Makefile文件：

LUA_SO = liblua.so

ALL_T = $(LUA_A) $(LUA_T) $(LUAC_T) $(LUA_SO)

$(LUA_SO): $(CORE_O) $(LIB_O)
    $(CC) -o $@ -shared $? -ldl -lm
```

编译安装:
make linux
make install


(有时需要修改ldconfig : /etc/ld.so.config.f/ 添加一个local.conf)
