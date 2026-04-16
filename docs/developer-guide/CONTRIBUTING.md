# Contributing to BrightS

Thank you for considering contributing to BrightS! We welcome contributions from the community.
[zh_CN](docs/CONTRIBUTING_zh_CN.md)

## How to Contribute

1. Fork the repository
2. Create a new branch for your feature or bug fix
3. Make your changes
4. Ensure your code follows the project's coding standards
5. Add tests if applicable
6. Submit a pull request

## Coding Standards

- Follow the existing code style in the project
- Write clear, descriptive comments
- Use meaningful variable and function names
- Keep functions focused and reasonably sized
- Handle errors appropriately

## Reporting Issues

Please use the GitHub issue tracker to report bugs or suggest features. Include:
- Clear description of the issue
- Steps to reproduce (for bugs)
- Expected vs actual behavior
- Relevant logs or error messages

## Development Setup

See [docs/build/build.md](docs/build/build.md) for instructions on setting up the development environment.

## Development Workflow

1. **选择任务**: 查看 [docs/TODO](docs/TODO) 文件选择开发任务
2. **创建分支**: `git checkout -b feature/your-feature-name`
3. **编写代码**: 遵循项目编码规范
4. **运行测试**: `make test` 确保所有测试通过
5. **提交代码**: `git commit -m "feat: add your feature"`
6. **推送分支**: `git push origin feature/your-feature-name`
7. **创建PR**: 在GitHub上创建Pull Request

## Code Quality Requirements

- **编译警告**: 必须为0（使用 `make -j$(nproc) 2>&1 | grep warning` 检查）
- **测试覆盖**: 新功能必须包含相应测试
- **性能**: 不能明显降低现有功能性能
- **文档**: 重要功能必须更新相关文档

## License

By contributing to BrightS, you agree that your contributions will be licensed under the project's license.
