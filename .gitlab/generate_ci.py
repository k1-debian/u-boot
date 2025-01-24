def generate_gitlab_ci(config_file_path, output_file_path):
    # 读取配置文件名
    with open(config_file_path, "r") as file:
        configs = [line.strip() for line in file if line.strip()]

    # 定义基础模板
    base_template = """
.build_base:
  stage: build
  resource_group: building
  script:
    - echo "开始编译 <$BUILD_CONFIG>..."
    - make O=$BUILD_DIR -C . $BUILD_CONFIG && make O=$BUILD_DIR
  artifacts:
    paths:
      - $BUILD_DIR/u-boot-with-spl.bin
      - $BUILD_DIR/u-boot-with-spl-mbr-gpt.bin
      - $BUILD_DIR/u-boot
      - $BUILD_DIR/spl/u-boot-spl
      - $BUILD_DIR/include/generated/
"""

    # 定义 job 模板
    job_template = """
build-{config}-job:
  extends: .build_base
  variables:
    BUILD_CONFIG: {config}
    BUILD_DIR: tmp.build.$BUILD_CONFIG
"""

    # 生成 CI 配置内容
    ci_config = base_template
    for config in configs:
        ci_config += job_template.format(config=config)

    # 将生成的配置保存到文件
    with open(output_file_path, "w") as output_file:
        output_file.write(ci_config)

    print(f"GitLab CI 配置已生成并保存到 {output_file_path}")


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 3:
        print("用法: python generate_ci.py <config_file_path> <output_file_path>")
        sys.exit(1)

    config_file_path = sys.argv[1]
    output_file_path = sys.argv[2]

    generate_gitlab_ci(config_file_path, output_file_path)

