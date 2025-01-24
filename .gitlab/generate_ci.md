# 说明

将boards.cfg里面的配置按照芯片生成对应的自动执行的job。

按照如下操作，可以将配置转换成gitlab job。

```
cat boards.cfg | grep ingenic | grep x2600 | awk '{print $1}' > x2600_config.txt
python generate_ci.py x2600_config.txt ci/x26xx.yml
```
