# Makefile模板

## 动态链接库模板
### 基础版

主要优点：
1. 使用vpath，可以自动寻找.cpp文件
2. 使用include，关联.h文件，.h文件更新，make会重新编译
3. 中间文件放进obj目录
4. 可以编译动态库和可执行文件，可以分开编译

```c++
SRCS_DIRS := $(shell find ./src/ -maxdepth 3 -type d)
SRCS_DIRS += $(shell find ./test/ -maxdepth 3 -type d)
SRCS := $(foreach dir,$(SRCS_DIRS),$(wildcard $(dir)/*.cpp))

MAKEROOT := $(shell pwd)
OBJ_DIR= $(MAKEROOT)/obj
$(shell mkdir -p "$(OBJ_DIR)")

OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

CC = g++
INCLUDES = -Iinclude \
		   -Itest \
		   -I.
LIBS = -lcurl -lpthread
CCFLAGS = -g -Wall -O0
OUTPUT = nacos-cli.out

vpath %.cpp $(SRCS_DIRS)

DEP:=$(OBJS:%.o=%.d)

all : $(OUTPUT)

$(OUTPUT) : $(OBJS)
	$(CC) $^ -o $@ $(INCLUDES) $(LIBS) $(CCFLAGS)

-include $(DEP)

$(OBJS) : $(OBJ_DIR)/%.o : %.cpp
	$(CC) -c $< -o $@ $(CCFLAGS) $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $@ -MF $(patsubst %.o, %.d, $@) $<

testcase : all
	SRCS = $(SRCS:testcase/*.cpp)

clean:
	rm -rf *.out
	rm -rf obj

.PHONY:clean
```

升级版
主要将.d的依赖文件单独存放
```c++
SRCS_DIRS := $(shell find ./src/ -maxdepth 3 -type d)
SRCS_DIRS += $(shell find ./test/ -maxdepth 3 -type d)
SRCS := $(foreach dir,$(SRCS_DIRS),$(wildcard $(dir)/*.cpp))

MAKEROOT = $(shell pwd)
OBJ_DIR = $(MAKEROOT)/obj
DEP_DIR = $(MAKEROOT)/dep

OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))
DEPS = $(patsubst %.o, $(DEP_DIR)/%.d, $(notdir $(OBJS)))

CC = g++
INCLUDES = -Iinclude \
		   -Itest \
		   -I.

LIBS = -lcurl -lpthread
CCFLAGS = -g -Wall -O0 -fPIC
OUTPUT = nacos-cli.out
OUTLIB = nacos-cli.so

vpath %.cpp $(SRCS_DIRS)

all :$(DEPS) $(OUTPUT) $(OUTLIB)

$(OUTPUT) : $(OBJS)
	$(info Linking $@ ...)
	@$(CC) $^ -o $@ $(INCLUDES) $(LIBS) $(CCFLAGS)

$(OUTLIB) : $(OBJS)
	$(info Linking $@ ...)
	@$(CC) $^ -o $@ $(INCLUDES) $(LIBS) $(CCFLAGS) -shared

$(OBJS) : $(OBJ_DIR)/%.o : %.cpp
	$(info Building $@ ...)
	@mkdir -p "$(OBJ_DIR)"
	@$(CC) -c $< -o $@ $(CCFLAGS) $(INCLUDES)

-include $(DEPS)
$(DEPS) : $(DEP_DIR)/%.d : %.cpp
	$(info Creating $<'s Dependencies ...)
	@mkdir -p "$(DEP_DIR)"
	@$(CC) $(CCFLAGS) $(INCLUDES) -MM -MT $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $<)) -MF $@ $<


testcase : all
	SRCS = $(SRCS:testcase/*.cpp)

clean:
	rm -rf *.out
	rm -rf *.so
	rm -rf $(OBJ_DIR)
	rm -rf $(DEP_DIR)

.PHONY:clean
```