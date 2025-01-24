BUILD_STATUS="成功"
err_msg="$CI_PROJECT_NAME 编译成功,请继续保持!"
title="gitlab-ci $CI_PROJECT_NAME 状态更新"
text="## 编译状态 :$BUILD_STATUS\n
 - 工程名称: $CI_PROJECT_NAME\n
 - 分支名称: $CI_COMMIT_BRANCH\n
 - 提交者: $CI_COMMIT_AUTHOR\n
 - 提交信息: $CI_COMMIT_MESSAGE\n
 - 提交日期: $CI_COMMIT_TIMESTAMP\n
 - 流水线: $CI_PIPELINE_URL\n
  $err_msg"

curl $CI_DINGTALK_WEBHOOK -H 'Content-Type: application/json' -d "{\"msgtype\": \"markdown\", \"markdown\":{\"title\":\"$title\", \"text\": \"$text\"}}"

