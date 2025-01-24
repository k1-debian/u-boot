#!/bin/bash
# 检查当前提交是否在本地执行过git-hooks脚本.

exit 0

# 暂时不检查.

keyword="ci-cd: prepare-commit-msg executed"
git log -1 | grep "$keyword" -q
if [ $? == 0 ]; then
	echo "git hooks 已经被执行."
	exit 0
fi

# 报告 hooks未安装警告.

BUILD_STATUS="本地未安装git-hooks警告"
err_msg="$CI_PROJECT_NAME 提交者本地没有正确设置git hooks, 请参考.gitlab/setup_git_hooks.md"
title="gitlab-ci $CI_PROJECT_NAME 状态更新"
text="## 编译状态 :$BUILD_STATUS\n
- 工程名称: $CI_PROJECT_NAME\n
- 分支名称: $CI_COMMIT_BRANCH\n
- 提交者: $CI_COMMIT_AUTHOR\n
- 提交信息: $CI_COMMIT_MESSAGE\n
- 提交日期: $CI_COMMIT_TIMESTAMP\n
- 流水线: $CI_PIPELINE_URL\n
- $err_msg"

curl $CI_DINGTALK_WEBHOOK -H 'Content-Type: application/json' -d "{\"msgtype\": \"markdown\", \"markdown\":{\"title\":\"$title\", \"text\": \"$text\"}}"

