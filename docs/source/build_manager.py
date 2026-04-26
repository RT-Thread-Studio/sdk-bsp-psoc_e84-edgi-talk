#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
中央构建管理器
完全基于 .github/versions.json 动态生成分支名称和构建配置
支持 Git Worktree 隔离构建，避免分支切换问题
"""

import os
import sys
import json
import shutil
import subprocess
import argparse
import platform
import re
from pathlib import Path
from typing import List, Dict, Optional, Union
from shutil import which
import yaml
from utils.i18n_config import I18nConfigManager

class VersionConfig:
    """版本配置类"""
    def __init__(self, config_dict: Dict):
        self.name = config_dict['name']
        self.display_name = config_dict['display_name']
        self.branch = config_dict['branch']
        self.url_path = config_dict['url_path']
        self.description = config_dict.get('description', '')

class BuildManager:
    """构建管理器"""
    
    def __init__(self):
        self.project_root = self._find_project_root()
        self.versions_file = self.project_root / '.github' / 'versions.json'
        self.docs_source = self.project_root / 'docs' / 'source'
        # 统一切换到新的构建输出根目录: source_build/html/<version>
        self.build_root = self.docs_source / 'source_build'
        self.worktrees_dir = self.build_root / 'worktrees'
        self.versions_dir = self.build_root / 'html'
        
        # 初始化国际化配置管理器
        config_path = self.docs_source / 'config.yaml'
        self.i18n_manager = I18nConfigManager(config_path)
        
    def _find_project_root(self) -> Path:
        """查找项目根目录"""
        current = Path.cwd()
        while current != current.parent:
            if (current / '.github' / 'versions.json').exists():
                return current
            current = current.parent
        raise FileNotFoundError("找不到 .github/versions.json 文件")
    
    def load_versions_config(self) -> Dict:
        """加载版本配置文件"""
        try:
            with open(self.versions_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
            print(f"✓ 加载版本配置: {[v['name'] for v in config.get('versions', [])]}")
            return config
        except Exception as e:
            print(f"✗ 无法加载版本配置: {e}")
            return {'versions': [], 'default_version': '', 'latest_version': ''}
    
    def get_version_configs(self) -> List[VersionConfig]:
        """获取版本配置列表"""
        config = self.load_versions_config()
        versions = []
        for version_dict in config.get('versions', []):
            versions.append(VersionConfig(version_dict))
        return versions
    
    def create_worktree(self, version_config: VersionConfig) -> Path:
        """为指定版本创建 Git worktree"""
        worktree_path = self.worktrees_dir / version_config.name
        
        # 获取当前分支
        current_branch = subprocess.run(
            ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
            capture_output=True, text=True, check=True
        ).stdout.strip()
        
        # 如果目标分支就是当前分支，直接使用当前目录
        if version_config.branch == current_branch:
            print(f"目标分支 {version_config.branch} 就是当前分支，使用当前目录")
            return Path.cwd()
        
        # 清理已存在的 worktree
        if worktree_path.exists():
            print(f"清理已存在的 worktree: {worktree_path}")
            try:
                subprocess.run(['git', 'worktree', 'remove', str(worktree_path)], 
                             check=True, capture_output=True)
            except subprocess.CalledProcessError:
                # 如果 worktree remove 失败，手动删除
                shutil.rmtree(worktree_path, ignore_errors=True)
        
        # 创建新的 worktree
        print(f"创建 worktree: {version_config.branch} -> {worktree_path}")
        subprocess.run([
            'git', 'worktree', 'add', 
            str(worktree_path), version_config.branch
        ], check=True)
        
        return worktree_path
    
    def build_docs_in_worktree(self, worktree_path: Path, version_config: VersionConfig) -> bool:
        """在 worktree 中构建文档"""
        print(f"在 worktree 中构建文档: {worktree_path}")
        
        # 检查 docs/source 目录是否存在
        if worktree_path == Path.cwd():
            # 如果是当前分支，使用主分支的 docs/source 目录
            docs_source_in_worktree = self.docs_source
        else:
            docs_source_in_worktree = worktree_path / 'docs' / 'source'
            if not docs_source_in_worktree.exists():
                print(f"⚠️  警告: {worktree_path} 中没有 docs/source 目录")
                print(f"   使用主分支的文档结构进行构建")
                # 复制主分支的文档结构
                main_docs = self.docs_source
                if main_docs.exists():
                    shutil.copytree(main_docs, docs_source_in_worktree, dirs_exist_ok=True)
                else:
                    print(f"✗ 错误: 主分支也没有 docs/source 目录")
                    return False
        
        # 切换到 worktree 目录（如果不是当前分支）
        if worktree_path != Path.cwd():
            os.chdir(worktree_path)
        
        try:
            # 读取项目名称用于 PDF 命名
            project_name = 'SDK_Docs'
            try:
                cfg_path = docs_source_in_worktree / 'config.yaml'
                if cfg_path.exists():
                    with open(cfg_path, 'r', encoding='utf-8') as f:
                        cfg = yaml.safe_load(f) or {}
                        project_name = (cfg.get('project', {}) or {}).get('name', project_name)
            except Exception:
                pass
            def _slugify(name: str) -> str:
                safe = []
                for ch in name:
                    if ch.isalnum() or ('\u4e00' <= ch <= '\u9fa5'):
                        safe.append(ch)
                    elif ch in [' ', '-', '_']:
                        safe.append('_' if ch == ' ' else ch)
                s = ''.join(safe).strip('_')
                return s or 'SDK_Docs'
            pdf_basename = _slugify(project_name) + '.pdf'
            # 运行文档生成脚本（如果存在）
            doc_generator = docs_source_in_worktree / 'doc_generator.py'
            if doc_generator.exists():
                print(f"运行文档生成脚本: {doc_generator}")
                subprocess.run([sys.executable, str(doc_generator)], 
                             cwd=str(docs_source_in_worktree), check=True)
            
            # 嵌入版本配置
            embed_script = docs_source_in_worktree / 'utils' / 'embed_version_config.py'
            if embed_script.exists():
                print(f"嵌入版本配置: {embed_script}")
                subprocess.run([sys.executable, str(embed_script)], 
                             cwd=str(docs_source_in_worktree), check=True)
            
            # 构建 HTML 文档 - 使用国际化配置管理器
            output_dir = self.build_root / 'html' / version_config.url_path
            print(f"构建 HTML 文档: {output_dir}")
            
            # 构建中文版文档
            print("构建中文版文档...")
            zh_output_dir = output_dir / 'zh'
            zh_config = self.i18n_manager.get_language_config('zh')
            zh_env = os.environ.copy()
            zh_env['SPHINX_MASTER_DOC'] = zh_config['index_filename'].replace('.rst', '')
            zh_env['SPHINX_MASTER_DOC_OVERRIDE'] = zh_config['index_filename'].replace('.rst', '')
            zh_env['SPHINX_LANGUAGE'] = 'zh_CN'
            # 确保中文locale环境变量
            zh_env['LANG'] = 'zh_CN.UTF-8'
            zh_env['LC_ALL'] = 'zh_CN.UTF-8'
            zh_env['LC_CTYPE'] = 'zh_CN.UTF-8'
            
            # 中文版构建时临时移动英文版文件，避免Sphinx读取
            moved_files = []
            try:
                # 从配置文件读取分类列表
                cfg_path = docs_source_in_worktree / 'config.yaml'
                if cfg_path.exists():
                    with open(cfg_path, 'r', encoding='utf-8') as f:
                        cfg = yaml.safe_load(f) or {}
                        categories = cfg.get('generation', {}).get('output_structure', [])
                        for category in categories:
                            # 临时移动英文版分类索引文件
                            en_index_file = docs_source_in_worktree / category / 'index.rst'
                            if en_index_file.exists():
                                temp_file = en_index_file.with_suffix('.rst.temp')
                                en_index_file.rename(temp_file)
                                moved_files.append((en_index_file, temp_file))
                                print(f"  临时移动英文版文件: {en_index_file} -> {temp_file}")
                
                # 临时移动英文版主索引文件
                en_main_index = docs_source_in_worktree / 'index.rst'
                if en_main_index.exists():
                    temp_file = en_main_index.with_suffix('.rst.temp')
                    en_main_index.rename(temp_file)
                    moved_files.append((en_main_index, temp_file))
                    print(f"  临时移动英文版文件: {en_main_index} -> {temp_file}")
                    
            except Exception as e:
                print(f"  警告: 移动英文版文件时出错: {e}")
            
            # 中文版构建时排除英文文档
            zh_env['SPHINX_EXCLUDE_PATTERNS'] = '*.md'
            
            print(f"中文版构建环境变量:")
            print(f"  LANG: {zh_env.get('LANG', 'N/A')}")
            print(f"  LC_ALL: {zh_env.get('LC_ALL', 'N/A')}")
            print(f"  SPHINX_LANGUAGE: {zh_env.get('SPHINX_LANGUAGE', 'N/A')}")
            print(f"  SPHINX_MASTER_DOC: {zh_env.get('SPHINX_MASTER_DOC', 'N/A')}")
            print(f"  SPHINX_EXCLUDE_PATTERNS: {zh_env.get('SPHINX_EXCLUDE_PATTERNS', 'N/A')}")
            print(f"  索引文件名: {zh_config['index_filename']}")
            
            subprocess.run([
                sys.executable, '-m', 'sphinx.cmd.build',
                '-b', 'html',
                '-D', 'language=zh_CN',
                '-D', 'master_doc=' + zh_config['index_filename'].replace('.rst', ''),
                str(docs_source_in_worktree),
                str(zh_output_dir)
            ], check=True, env=zh_env)
            
            # 恢复临时移动的英文版文件
            for original_file, temp_file in moved_files:
                try:
                    if temp_file.exists():
                        temp_file.rename(original_file)
                        print(f"  恢复英文版文件: {temp_file} -> {original_file}")
                except Exception as e:
                    print(f"  警告: 恢复文件时出错 {temp_file}: {e}")
            
            # 检查翻译文件是否生成
            translations_file = zh_output_dir / '_static' / 'translations.js'
            if translations_file.exists():
                print(f"✓ 中文翻译文件已生成: {translations_file}")
                # 检查翻译文件内容
                with open(translations_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    if 'zh_Hans_CN' in content or 'zh_CN' in content:
                        print("✓ 翻译文件包含中文locale信息")
                    else:
                        print("⚠️  翻译文件可能不包含正确的中文locale信息")
            else:
                print("⚠️  中文翻译文件未生成")
            
            # 构建英文版文档
            print("构建英文版文档...")
            en_output_dir = output_dir / 'en'
            en_config = self.i18n_manager.get_language_config('en')
            en_env = os.environ.copy()
            en_env['SPHINX_MASTER_DOC'] = en_config['index_filename'].replace('.rst', '')
            en_env['SPHINX_MASTER_DOC_OVERRIDE'] = en_config['index_filename'].replace('.rst', '')
            en_env['SPHINX_LANGUAGE'] = 'en'
            # 确保英文locale环境变量
            en_env['LANG'] = 'en_US.UTF-8'
            en_env['LC_ALL'] = 'en_US.UTF-8'
            en_env['LC_CTYPE'] = 'en_US.UTF-8'
            
            # 英文版构建时临时移动中文版文件，避免Sphinx读取
            moved_files_en = []
            try:
                # 从配置文件读取分类列表
                cfg_path = docs_source_in_worktree / 'config.yaml'
                if cfg_path.exists():
                    with open(cfg_path, 'r', encoding='utf-8') as f:
                        cfg = yaml.safe_load(f) or {}
                        categories = cfg.get('generation', {}).get('output_structure', [])
                        for category in categories:
                            # 临时移动中文版分类索引文件
                            zh_index_file = docs_source_in_worktree / category / 'index_zh.rst'
                            if zh_index_file.exists():
                                temp_file = zh_index_file.with_suffix('.rst.temp')
                                zh_index_file.rename(temp_file)
                                moved_files_en.append((zh_index_file, temp_file))
                                print(f"  临时移动中文版文件: {zh_index_file} -> {temp_file}")
                
                # 临时移动中文版主索引文件
                zh_main_index = docs_source_in_worktree / 'index_zh.rst'
                if zh_main_index.exists():
                    temp_file = zh_main_index.with_suffix('.rst.temp')
                    zh_main_index.rename(temp_file)
                    moved_files_en.append((zh_main_index, temp_file))
                    print(f"  临时移动中文版文件: {zh_main_index} -> {temp_file}")
                    
            except Exception as e:
                print(f"  警告: 移动中文版文件时出错: {e}")
            
            # 英文版构建时排除中文文档
            en_env['SPHINX_EXCLUDE_PATTERNS'] = '*_zh.md'
            
            print(f"英文版构建环境变量:")
            print(f"  LANG: {en_env.get('LANG', 'N/A')}")
            print(f"  LC_ALL: {en_env.get('LC_ALL', 'N/A')}")
            print(f"  SPHINX_LANGUAGE: {en_env.get('SPHINX_LANGUAGE', 'N/A')}")
            print(f"  SPHINX_EXCLUDE_PATTERNS: {en_env.get('SPHINX_EXCLUDE_PATTERNS', 'N/A')}")
            
            subprocess.run([
                sys.executable, '-m', 'sphinx.cmd.build',
                '-b', 'html',
                '-D', 'master_doc=' + en_config['index_filename'].replace('.rst', ''),
                '-D', 'language=en',
                str(docs_source_in_worktree),
                str(en_output_dir)
            ], check=True, env=en_env)
            
            # 恢复临时移动的中文版文件
            for original_file, temp_file in moved_files_en:
                try:
                    if temp_file.exists():
                        temp_file.rename(original_file)
                        print(f"  恢复中文版文件: {temp_file} -> {original_file}")
                except Exception as e:
                    print(f"  警告: 恢复文件时出错 {temp_file}: {e}")
            
            # 检查翻译文件是否生成，如果没有则手动创建
            translations_file = en_output_dir / '_static' / 'translations.js'
            if translations_file.exists():
                print(f"✓ 英文翻译文件已生成: {translations_file}")
                # 检查翻译文件内容
                with open(translations_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    if 'en_US' in content or 'en' in content:
                        print("✓ 翻译文件包含英文locale信息")
                    else:
                        print("⚠️  翻译文件可能不包含正确的英文locale信息")
            else:
                print("⚠️  英文翻译文件未生成，手动创建...")
                # 手动创建英文翻译文件
                en_translations_content = '''const TRANSLATIONS = {
    "locale": "en_US",
    "messages": {
        "Search": "Search",
        "Searching": "Searching",
        "Search Results": "Search Results",
        "Search finished, found %s page(s) matching the search query.": "Search finished, found %s page(s) matching the search query.",
        "Search didn't return any results. Please try again with different keywords.": "Search didn't return any results. Please try again with different keywords.",
        "Search Results for": "Search Results for",
        "Searching for": "Searching for",
        "Search": "Search",
        "Searching": "Searching",
        "Search Results": "Search Results"
    }
};
'''
                # 确保目录存在
                translations_file.parent.mkdir(parents=True, exist_ok=True)
                with open(translations_file, 'w', encoding='utf-8') as f:
                    f.write(en_translations_content)
                print(f"✓ 已手动创建英文翻译文件: {translations_file}")
            
            # 合并文档集到统一目录
            print("合并文档集...")
            self._merge_docs_with_i18n(zh_output_dir, en_output_dir, output_dir)
            
            # 生成版本配置（注入项目源目录片段与复制文件规则）
            # 从 docs/source/config.yaml 读取 repository.projects_dir，并转换为仓库内相对路径片段
            projects_dir_web = ''
            copy_files_list = []
            try:
                cfg_path = docs_source_in_worktree / 'config.yaml'
                if cfg_path.exists():
                    with open(cfg_path, 'r', encoding='utf-8') as f:
                        repo_cfg = yaml.safe_load(f) or {}
                        pdir = ((repo_cfg.get('repository', {}) or {}).get('projects_dir', '') or '').replace('\\','/')
                        # 若是相对路径如 ../../project，则仅取末段 "project"
                        if pdir:
                            parts = [seg for seg in pdir.split('/') if seg and seg != '..' and seg != '.']
                            if parts:
                                projects_dir_web = '/'.join(parts[-1:])
                        copy_files_list = ((repo_cfg.get('generation', {}) or {}).get('copy_files', []) or [])
            except Exception:
                pass

            self._generate_version_config(output_dir, version_config, projects_dir_web, copy_files_list)

            # 构建 PDF（仅使用增强版V2生成器，生成中英文两个版本）
            pdf_file = None
            from pdf_generator_enhanced_v2 import PDFGeneratorV2
            print("使用增强版V2 PDF生成器...")
            pdf_generator = PDFGeneratorV2(output_dir, output_dir / '_static')
            # 中文
            if pdf_generator.generate_pdf(project_name, language="zh"):
                static_dir = output_dir / '_static'
                candidate_pdf = static_dir / f'{project_name}.pdf'
                if candidate_pdf.exists():
                    pdf_file = candidate_pdf
                    print(f"✓ 中文PDF生成成功: {pdf_file}")
                else:
                    print("⚠️  中文PDF文件未找到")
            else:
                print("⚠️  中文PDF生成失败")
            # 英文
            print("正在生成英文版本PDF...")
            if pdf_generator.generate_pdf(project_name, language="en"):
                static_dir = output_dir / '_static'
                # 英文 PDF 名称使用下划线替换空格
                en_pdf = static_dir / f"{project_name.replace(' ', '_')}_EN.pdf"
                if en_pdf.exists():
                    print(f"✓ 英文PDF生成成功: {en_pdf}")
                else:
                    print("⚠️  英文PDF文件未找到")
            else:
                print("⚠️  英文PDF生成失败")

            # 将 PDF 复制到 HTML 的 _static 目录，供在线下载
            static_dir = output_dir / '_static'
            static_dir.mkdir(exist_ok=True)
            
            if pdf_file and pdf_file.exists():
                target_pdf = static_dir / pdf_basename
                try:
                    # 避免源与目标为同一文件时复制报错
                    if pdf_file.resolve() != target_pdf.resolve():
                        shutil.copy2(pdf_file, target_pdf)
                        print(f"✓ 生成并复制 PDF: {pdf_file.name} -> {target_pdf}")
                    else:
                        print(f"✓ PDF 已在目标位置: {target_pdf}")
                except Exception as copy_err:
                    print(f"⚠️  复制 PDF 时出现问题（已忽略）：{copy_err}")
                # 兼容默认名称，额外复制一份 sdk-docs.pdf，便于前端 file:// 环境无需获取项目信息
                fallback_pdf = static_dir / 'sdk-docs.pdf'
                try:
                    shutil.copy2(pdf_file, fallback_pdf)
                except Exception:
                    pass
            else:
                print("⚠️  未生成 PDF，创建占位文件")
                # 创建一个占位PDF文件，避免下载按钮不显示
                placeholder_pdf = static_dir / 'sdk-docs.pdf'
                with open(placeholder_pdf, 'w', encoding='utf-8') as f:
                    f.write("PDF文件正在生成中，请稍后重试...")
            
            # 写入项目信息，供前端读取文件名
            project_info = {
                'projectName': project_name,
                'pdfFileName': pdf_basename
            }
            with open(static_dir / 'project_info.json', 'w', encoding='utf-8') as f:
                json.dump(project_info, f, ensure_ascii=False)
            # 兼容 file:// 环境：同时输出 JS 版本，供页面直接读取
            try:
                with open(static_dir / 'project_info.js', 'w', encoding='utf-8') as f_js:
                    f_js.write('window.projectInfo = ' + json.dumps(project_info, ensure_ascii=False) + ';\n')
            except Exception:
                pass
            
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"✗ 构建失败: {e}")
            return False
        finally:
            # 恢复到原始目录（如果不是当前分支）
            if worktree_path != Path.cwd():
                os.chdir(self.project_root)
    
    def _generate_version_config(self, output_dir: Path, version_config: VersionConfig, projects_dir_web: str = '', copy_files: list = None):
        """生成版本切换配置文件
        projects_dir_web: 仓库内项目根路径（URL 片段），例如 "project" 或 "projects/examples"
        """
        config = self.load_versions_config()
        
        # 创建版本配置 JSON 文件 - 修复格式
        version_config_file = output_dir / 'version_config.json'
        with open(version_config_file, 'w', encoding='utf-8') as f:
            json.dump(config, f, ensure_ascii=False, indent=2)
        
        # 创建版本信息 HTML 文件
        version_info_file = output_dir / 'version_info.html'
        version_info_html = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>版本信息</title>
</head>
<body>
    <script>
        window.versionInfo = {{
            "name": "{version_config.name}",
            "display_name": "{version_config.display_name}",
            "branch": "{version_config.branch}",
            "url_path": "{version_config.url_path}",
            "description": "{version_config.description}"
        }};
    </script>
</body>
</html>"""
        
        with open(version_info_file, 'w', encoding='utf-8') as f:
            f.write(version_info_html)
        
        # 同时创建 _static 目录下的配置文件
        static_dir = output_dir / '_static'
        static_dir.mkdir(exist_ok=True)
        static_config_file = static_dir / 'version_config.json'
        with open(static_config_file, 'w', encoding='utf-8') as f:
            json.dump(config, f, ensure_ascii=False, indent=2)
        # 生成可直接加载的 JS，提供 window.versionInfo（含当前版本信息）
        version_info_js = static_dir / 'version_info.js'
        version_info_obj = {
            "name": version_config.name,
            "display_name": version_config.display_name,
            "branch": version_config.branch,
            "url_path": version_config.url_path,
            "description": version_config.description,
            "projectsDir": projects_dir_web or '',
            "copyFiles": copy_files or []
        }
        with open(version_info_js, 'w', encoding='utf-8') as f:
            f.write("window.versionInfo = " + json.dumps(version_info_obj, ensure_ascii=False) + ";\n")
        
        print(f"✓ 生成版本配置文件: {version_config_file}")
        print(f"✓ 生成静态配置文件: {static_config_file}")
    
    
    def _merge_docs_with_i18n(self, zh_dir: Path, en_dir: Path, output_dir: Path):
        """使用国际化配置合并中英文文档集"""
        import shutil
        
        # 创建输出目录
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # 第一步：复制英文版文档（保持原名，无后缀表示英文）
        print("复制英文版文档...")
        self._copy_docs_with_html_fix(en_dir, output_dir, 'en')
        
        # 第二步：复制中文版文档（添加_zh后缀）
        print("复制中文版文档...")
        self._copy_docs_with_html_fix(zh_dir, output_dir, 'zh')
        
        # 清理临时目录
        shutil.rmtree(zh_dir, ignore_errors=True)
        shutil.rmtree(en_dir, ignore_errors=True)
        
        print("✓ 文档集合并完成")
        print(f"  - 中文版文件：添加 _zh 后缀（如 index_zh.html, README_zh.html）")
        print(f"  - 英文版文件：保持原名（如 index.html, README.html）")
    
    def _copy_docs_with_html_fix(self, source_dir: Path, target_dir: Path, language: str):
        """复制文档并修复HTML文件的语言配置"""
        import shutil
        
        # 确保目标目录存在
        target_dir.mkdir(parents=True, exist_ok=True)
        
        for item in source_dir.iterdir():
            if item.is_file():
                if item.name.endswith('.html'):
                    # Sphinx may still emit both README.html and README_zh.html in
                    # each temporary build. Pick the correct variant to avoid one
                    # language overwriting the other during the final merge.
                    new_name = self._get_i18n_html_target_name(item, language)
                    if not new_name:
                        continue
                    target_file = target_dir / new_name
                    self._fix_html_language(item, target_file, language)
                else:
                    # 非HTML文件直接复制
                    shutil.copy2(item, target_dir / item.name)
            elif item.is_dir() and not item.name.startswith('.'):
                # 只处理非隐藏目录，跳过 .doctrees 等Sphinx内部目录
                target_subdir = target_dir / item.name
                self._copy_docs_with_html_fix(item, target_subdir, language)

    def _get_i18n_html_target_name(self, html_file: Path, language: str) -> Optional[str]:
        """Return merged HTML filename for a language, or None for duplicates."""
        stem = html_file.stem
        if language == 'zh':
            if stem.endswith('_zh'):
                return html_file.name
            # If the real Chinese page exists, do not let the English page
            # collide with it as *_zh.html in the merged output.
            if html_file.with_name(f'{stem}_zh.html').exists():
                return None
            return f'{stem}_zh.html'

        if stem.endswith('_zh'):
            base_stem = stem[:-3]
            # If the real English page exists, skip the Chinese duplicate.
            if html_file.with_name(f'{base_stem}.html').exists():
                return None
            # Chinese-only fallback pages become the English default filename so
            # links normalized from *_zh.html to *.html still resolve.
            return f'{base_stem}.html'

        return html_file.name
    
    def _fix_html_language(self, source_file: Path, target_file: Path, language: str):
        """修复HTML文件的语言配置"""
        try:
            with open(source_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 修复语言属性
            if language == 'en':
                # 英文版修复
                content = re.sub(r'lang="zh-CN"', 'lang="en"', content)
                content = re.sub(r'placeholder="搜索文档"', 'placeholder="Search documentation"', content)
                content = re.sub(r'aria-label="搜索文档"', 'aria-label="Search documentation"', content)
                content = re.sub(r'aria-label="导航菜单"', 'aria-label="Navigation menu"', content)
                content = re.sub(r'aria-label="移动版导航菜单"', 'aria-label="Mobile navigation menu"', content)
                content = re.sub(r'aria-label="页面导航"', 'aria-label="Page navigation"', content)
                content = re.sub(r'aria-label="页脚"', 'aria-label="Footer"', content)
                
                # 修复链接指向
                content = re.sub(r'href="([^"]*)_zh\.html"', r'href="\1.html"', content)
                content = re.sub(r'href="([^"]*)/index_zh\.html"', r'href="\1/index.html"', content)
                
                # 修复目录结构中的链接
                content = re.sub(r'href="([^"]*)_zh\.html#', r'href="\1.html#', content)
                
            else:
                # 中文版保持原样，但确保语言属性正确
                content = re.sub(r'lang="en"', 'lang="zh-CN"', content)
                # 确保中文版链接指向中文版文件
                content = re.sub(r'href="([^"]*)(?<!_zh)\.html"', r'href="\1_zh.html"', content)
                content = re.sub(r'href="([^"]*)/index\.html"', r'href="\1/index_zh.html"', content)
                # 修复搜索框文本
                content = re.sub(r'placeholder="Search documentation"', 'placeholder="搜索文档"', content)
                content = re.sub(r'aria-label="Search documentation"', 'aria-label="搜索文档"', content)
                content = re.sub(r'aria-label="Navigation menu"', 'aria-label="导航菜单"', content)
                content = re.sub(r'aria-label="Mobile navigation menu"', 'aria-label="移动版导航菜单"', content)
                content = re.sub(r'aria-label="Page navigation"', 'aria-label="页面导航"', content)
                content = re.sub(r'aria-label="Footer"', 'aria-label="页脚"', content)
            
            # 写入修复后的文件
            with open(target_file, 'w', encoding='utf-8') as f:
                f.write(content)
                
        except Exception as e:
            print(f"⚠️  修复HTML文件语言配置失败: {e}")
            # 如果修复失败，直接复制原文件
            shutil.copy2(source_file, target_file)
    
    def copy_build_result(self, worktree_path: Path, version_config: VersionConfig):
        """就地构建后无需复制，保持接口以兼容调用方"""
        target_dir = self.versions_dir / version_config.url_path
        if target_dir.exists():
            print(f"✓ 构建结果已在目标目录: {target_dir}")
            return True
        else:
            print(f"✗ 目标目录不存在: {target_dir}")
            return False

    def _generate_pdf_latex(self, docs_source_in_worktree: Path, version_config: VersionConfig) -> Optional[Path]:
        """使用传统LaTeX方法生成PDF（作为回退方案）"""
        self._ensure_pdf_dependencies()
        pdf_file = None
        
        try:
            # 尝试 latexpdf 构建器
            latexpdf_dir = self.build_root / 'latexpdf' / version_config.url_path
            print(f"尝试使用 latexpdf 构建: {latexpdf_dir}")
            subprocess.run([
                sys.executable, '-m', 'sphinx.cmd.build',
                '-b', 'latexpdf',
                str(docs_source_in_worktree),
                str(latexpdf_dir)
            ], check=True)

            # 预期输出：conf.py 设定主文档名 sdk-docs.tex -> sdk-docs.pdf
            candidate = latexpdf_dir / 'sdk-docs.pdf'
            if candidate.exists():
                pdf_file = candidate
            else:
                # 回退查找任意 pdf
                pdf_candidates = list(latexpdf_dir.glob('*.pdf'))
                if pdf_candidates:
                    pdf_file = pdf_candidates[0]
        except subprocess.CalledProcessError:
            # 回退到 latex + 编译链
            latex_dir = self.build_root / 'latex' / version_config.url_path
            print(f"latexpdf 失败，回退到 LaTeX 构建: {latex_dir}")
            subprocess.run([
                sys.executable, '-m', 'sphinx.cmd.build',
                '-b', 'latex',
                str(docs_source_in_worktree),
                str(latex_dir)
            ], check=True)

            try:
                tex_files = list(latex_dir.glob('*.tex'))
                main_tex = None
                # 优先使用 conf.py 指定的 sdk-docs.tex
                candidate_tex = latex_dir / 'sdk-docs.tex'
                if candidate_tex.exists():
                    main_tex = candidate_tex
                elif tex_files:
                    main_tex = tex_files[0]

                if main_tex:
                    # latexmk -> tectonic -> pdflatex
                    compiled = False
                    try:
                        subprocess.run(['latexmk', '-pdf', '-silent', '-interaction=nonstopmode', str(main_tex.name)], cwd=str(latex_dir), check=True)
                        compiled = True
                    except Exception:
                        try:
                            subprocess.run(['tectonic', str(main_tex.name)], cwd=str(latex_dir), check=True)
                            compiled = True
                        except Exception:
                            try:
                                subprocess.run(['pdflatex', '-interaction=nonstopmode', str(main_tex.name)], cwd=str(latex_dir), check=True)
                                subprocess.run(['pdflatex', '-interaction=nonstopmode', str(main_tex.name)], cwd=str(latex_dir), check=True)
                                compiled = True
                            except Exception:
                                pass

                    if compiled:
                        # 优先 sdk-docs.pdf
                        candidate_pdf = latex_dir / 'sdk-docs.pdf'
                        if candidate_pdf.exists():
                            pdf_file = candidate_pdf
                        else:
                            pdf_candidates = list(latex_dir.glob('*.pdf'))
                            if pdf_candidates:
                                pdf_file = pdf_candidates[0]
            except Exception as e:
                print(f"⚠️  LaTeX 回退编译失败: {e}")
        
        return pdf_file

    def _ensure_pdf_dependencies(self):
        """尽力确保本机具备 PDF 构建依赖。优先 tectonic，其次 latexmk/texlive，再次 pdflatex。
        以非交互方式尝试安装，失败仅警告不报错。
        """
        def _exists(cmd: str) -> bool:
            return which(cmd) is not None

        have_tool = _exists('tectonic') or _exists('latexmk') or _exists('pdflatex')
        have_xelatex = _exists('xelatex')
        if have_tool and have_xelatex:
            return

        system = platform.system().lower()
        print("尝试安装 PDF 构建依赖...")
        try:
            if system == 'windows':
                if _exists('choco'):
                    try:
                        subprocess.run(['choco', 'install', 'tectonic', '-y'], check=False)
                    except Exception:
                        pass
                    try:
                        subprocess.run(['choco', 'install', 'miktex', '-y'], check=False)
                    except Exception:
                        pass
            elif system == 'linux':
                if _exists('apt-get'):
                    subprocess.run(['sudo', 'apt-get', 'update'], check=False)
                    subprocess.run(['sudo', 'apt-get', 'install', '-y', 'tectonic'], check=False)
                    subprocess.run(['sudo', 'apt-get', 'install', '-y', 'latexmk', 'texlive-latex-recommended', 'texlive-latex-extra', 'texlive-fonts-recommended', 'texlive-xetex', 'fonts-noto-cjk'], check=False)
            elif system == 'darwin':
                if _exists('brew'):
                    subprocess.run(['brew', 'install', 'tectonic'], check=False)
                    subprocess.run(['brew', 'install', 'basictex'], check=False)
                    subprocess.run(['sudo', 'tlmgr', 'update', '--self'], check=False)
                    subprocess.run(['sudo', 'tlmgr', 'install', 'latexmk', 'xetex'], check=False)
        except Exception as e:
            print(f"PDF 依赖安装尝试失败: {e}")
    
    def cleanup_worktree(self, worktree_path: Path):
        """清理 worktree：仅对 source_build/worktrees 下的有效 worktree 执行删除"""
        if not worktree_path.exists():
            return

        # 仅在我们的临时 worktrees 根目录下才允许删除
        try:
            worktree_root = self.worktrees_dir.resolve()
            candidate = worktree_path.resolve()
            is_under_root = str(candidate).startswith(str(worktree_root))
        except Exception:
            is_under_root = False

        if not is_under_root:
            # 避免误删非临时目录（例如当前仓库根或任意外部路径）
            return

        # 在删除之前确认它是一个已登记的 git worktree
        is_git_worktree = False
        try:
            listed = subprocess.run(['git', 'worktree', 'list'], capture_output=True, text=True, check=True).stdout
            is_git_worktree = str(candidate) in listed
        except Exception:
            pass

        if is_git_worktree:
            # 尝试优先用 git worktree remove --force
            for args in (["git", "worktree", "remove", "--force", str(candidate)],
                         ["git", "worktree", "remove", str(candidate)]):
                try:
                    subprocess.run(args, check=True, capture_output=True)
                    print(f"✓ 清理 worktree: {worktree_path}")
                    return
                except subprocess.CalledProcessError:
                    continue

        # 兜底：非登记 worktree 或命令失败，做文件系统级别删除
        shutil.rmtree(candidate, ignore_errors=True)
    
    def build_all_versions(self, clean=False):
        """构建所有版本"""
        print("=" * 60)
        print("开始构建所有版本")
        print("=" * 60)
        
        if clean:
            print("清理构建目录...")
            if self.build_root.exists():
                shutil.rmtree(self.build_root)
        
        # 确保构建目录存在
        self.build_root.mkdir(parents=True, exist_ok=True)
        self.versions_dir.mkdir(parents=True, exist_ok=True)
        
        # 加载版本配置
        versions = self.get_version_configs()
        print(f"✓ 加载版本配置: {[v.name for v in versions]}")
        
        success_count = 0
        total_count = len(versions)
        
        for version_config in versions:
            print("\n" + "=" * 40)
            print(f"构建版本: {version_config.display_name} ({version_config.branch})")
            print("=" * 40)
            
            # 创建或获取 worktree
            worktree_path = self.create_worktree(version_config)
            if not worktree_path:
                print(f"✗ 无法为版本 {version_config.display_name} 创建 worktree")
                continue
            
            try:
                # 构建文档
                if self.build_docs_in_worktree(worktree_path, version_config):
                    # 复制构建结果
                    if self.copy_build_result(worktree_path, version_config):
                        success_count += 1
                        print(f"✓ 版本 {version_config.display_name} 构建成功")
                    else:
                        print(f"✗ 版本 {version_config.display_name} 复制失败")
                else:
                    print(f"✗ 版本 {version_config.display_name} 构建失败")
            finally:
                # 清理 worktree
                self.cleanup_worktree(worktree_path)
        
        # 创建统一入口页面，指向新的根目录结构
        self.create_unified_index()
        # 在 html 根目录下创建 index.html 指向默认版本
        self.create_versions_root_index()
        
        print("\n" + "=" * 60)
        print(f"构建完成: {success_count}/{total_count} 个版本成功")
        print("=" * 60)
        
        return success_count == total_count
    
    def create_unified_index(self):
        """创建统一的文档入口页面"""
        config = self.load_versions_config()
        versions = config.get('versions', [])
        default_version = config.get('default_version', '')
        latest_version = config.get('latest_version', '')
        
        # 找到默认版本的 URL 路径
        default_url = 'latest'
        for version in versions:
            if version['name'] == default_version:
                default_url = version['url_path']
                break
        
        # 创建根目录的 index.html
        index_html = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SDK 文档</title>
    <meta http-equiv="refresh" content="0; url=./versions/{default_url}/index.html">
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }}
        .container {{
            text-align: center;
            background: rgba(255, 255, 255, 0.1);
            padding: 40px;
            border-radius: 12px;
            backdrop-filter: blur(10px);
        }}
        .spinner {{
            border: 3px solid rgba(255, 255, 255, 0.3);
            border-top: 3px solid white;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto 20px;
        }}
        @keyframes spin {{
            0% {{ transform: rotate(0deg); }}
            100% {{ transform: rotate(360deg); }}
        }}
        h1 {{
            margin: 0 0 10px 0;
            font-size: 24px;
        }}
        p {{
            margin: 0;
            opacity: 0.9;
        }}
        a {{
            color: white;
            text-decoration: underline;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="spinner"></div>
        <h1>SDK 文档</h1>
        <p>正在跳转到文档首页...</p>
        <p><a href="./versions/{default_url}/index.html">如果页面没有自动跳转，请点击这里</a></p>
    </div>
</body>
</html>"""
        
        index_file = self.build_root / 'html' / 'index.html'
        index_file.parent.mkdir(parents=True, exist_ok=True)
        
        with open(index_file, 'w', encoding='utf-8') as f:
            f.write(index_html)
        
        print(f"✓ 创建统一入口页面: {index_file}")

    def create_versions_root_index(self):
        """在versions目录下创建根页面"""
        config = self.load_versions_config()
        versions = config.get('versions', [])
        default_version = config.get('default_version', '')
        latest_version = config.get('latest_version', '')
        
        # 找到默认版本的 URL 路径
        default_url = 'latest'
        for version in versions:
            if version['name'] == default_version:
                default_url = version['url_path']
                break
        
        # 创建versions目录的index.html
        versions_index_html = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SDK 文档 - 版本列表</title>
    <meta http-equiv="refresh" content="0; url=./{default_url}/index.html">
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }}
        .container {{
            text-align: center;
            background: rgba(255, 255, 255, 0.1);
            padding: 40px;
            border-radius: 12px;
            backdrop-filter: blur(10px);
        }}
        .spinner {{
            border: 3px solid rgba(255, 255, 255, 0.3);
            border-top: 3px solid white;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto 20px;
        }}
        @keyframes spin {{
            0% {{ transform: rotate(0deg); }}
            100% {{ transform: rotate(360deg); }}
        }}
        h1 {{
            margin: 0 0 10px 0;
            font-size: 24px;
        }}
        p {{
            margin: 0;
            opacity: 0.9;
        }}
        a {{
            color: white;
            text-decoration: underline;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="spinner"></div>
        <h1>SDK 文档 - 版本列表</h1>
        <p>正在跳转到文档首页...</p>
        <p><a href="./{default_url}/index.html">如果页面没有自动跳转，请点击这里</a></p>
    </div>
</body>
</html>"""
        
        versions_index_file = self.versions_dir / 'index.html'
        versions_index_file.parent.mkdir(parents=True, exist_ok=True)
        
        with open(versions_index_file, 'w', encoding='utf-8') as f:
            f.write(versions_index_html)
        
        print(f"✓ 创建versions目录根页面: {versions_index_file}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="中央构建管理器")
    parser.add_argument('--clean', action='store_true', help='清理构建目录')
    parser.add_argument('--list-versions', action='store_true', help='列出所有版本')
    parser.add_argument('--check-config', action='store_true', help='检查版本配置')
    
    args = parser.parse_args()
    
    try:
        manager = BuildManager()
        
        if args.list_versions:
            versions = manager.get_version_configs()
            print("版本列表:")
            for version in versions:
                print(f"  - {version.display_name} ({version.name}) -> {version.branch}")
            return
        
        if args.check_config:
            config = manager.load_versions_config()
            print("版本配置检查:")
            print(f"  默认版本: {config.get('default_version', 'N/A')}")
            print(f"  最新版本: {config.get('latest_version', 'N/A')}")
            print(f"  版本数量: {len(config.get('versions', []))}")
            return
        
        # 构建所有版本
        success = manager.build_all_versions(clean=args.clean)
        
        if success:
            print("\n🎉 所有版本构建成功!")
            print(f"📁 文档位置: {manager.versions_dir}")
        else:
            print("\n❌ 部分版本构建失败!")
            sys.exit(1)
            
    except Exception as e:
        print(f"✗ 构建管理器错误: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 
