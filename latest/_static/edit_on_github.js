(function(){
  function getMetaEditBase(){
    // 从 Sphinx html_context 注入的值读取
    try {
      var metas = document.getElementsByTagName('meta');
      for (var i=0;i<metas.length;i++){
        if (metas[i].getAttribute('name') === 'edit-base-url'){
          return (metas[i].getAttribute('content') || '').replace(/\/+$/, '/')
        }
      }
    } catch(e) {}
    return '';
  }

  function injectMetaFromContext(){
    // conf.py 会把 edit_base_url 放到 html_context；我们在模板层插一条 meta
    if (window && window.SPHINX_EDIT_BASE_URL && !document.querySelector('meta[name="edit-base-url"]')){
      var m = document.createElement('meta');
      m.setAttribute('name','edit-base-url');
      m.setAttribute('content', window.SPHINX_EDIT_BASE_URL);
      document.head.appendChild(m);
    }
  }

  function isEditablePage(){
    // 检查当前页面是否对应 Markdown 文件
    var path = location.pathname;
    var urlPath = (window.versionInfo && window.versionInfo.url_path) ? window.versionInfo.url_path : 'latest';
    var marker = '/' + urlPath + '/';
    var idx = path.indexOf(marker);
    
    if (idx < 0) return false; // 不在文档路径中
    
    var relativePath = path.substring(idx + marker.length);
    
    // 过滤掉索引页面和特殊页面
    if (!relativePath || relativePath === '' || relativePath === 'index.html') return false;
    if (relativePath.match(/^(search|genindex|modindex)\.html$/)) return false;
    if (relativePath.match(/\/(index\.html)?$/)) return false; // 目录索引页
    
    // 检查是否在项目目录结构中（包含至少两级路径，如 basic/project_name/README.html）
    var pathParts = relativePath.split('/');
    if (pathParts.length < 2) return false;
    
    // 检查对应的源文件是否是 Markdown 文件
    // 通过检查 copyFiles 中是否有对应的 .md 文件来判断
    var copyFiles = (window.versionInfo && window.versionInfo.copyFiles) ? window.versionInfo.copyFiles : [];
    var fileName = pathParts[pathParts.length - 1].replace(/\.html$/i, '').toLowerCase();
    
    // 检查 copyFiles 中是否有对应的 .md 文件
    var hasMarkdownFile = copyFiles.some(function(file) {
      var fileBaseName = String(file || '').replace(/\.md$/i, '').toLowerCase();
      return fileBaseName === fileName;
    });
    
    return hasMarkdownFile;
  }

  function detectCurrentLanguage() {
    var htmlLang = document.documentElement.getAttribute('lang');
    var currentPath = window.location.pathname;
    if ((htmlLang === 'zh-CN') && (currentPath.endsWith('_zh.html'))) return 'zh';
    if ((htmlLang === 'en') && (currentPath.endsWith('.html'))) return 'en';
    
    return 'en'; // 默认英文
  }

  function buildEditUrl(){
    var base = getMetaEditBase();
    // 规范化 base，确保为绝对 URL，避免生成相对路径
    function normalizeBase(s){
      if (!s) return '';
      s = (s + '').trim();
      if (/^https?:\/\//i.test(s)) return s.replace(/\/+$/, '/')
      if (/github\.com\//i.test(s)) return ('https://' + s).replace(/\/+$/, '/');
      return '';
    }
    base = normalizeBase(base);
    if (!base) {
      // 在 file:// 环境下没有配置 base，则不显示按钮
      if (location && location.protocol === 'file:') return '';
    }
    if (!base) return '';
    
    var branch = (window.versionInfo && window.versionInfo.branch) ? window.versionInfo.branch : 'master';
    // 当前页面在版本目录后的相对路径
    var path = location.pathname;
    var urlPath = (window.versionInfo && window.versionInfo.url_path) ? window.versionInfo.url_path : 'latest';
    // 仅截取版本目录后的相对路径（不含域名与构建根）
    var marker = '/' + urlPath + '/';
    var idx = path.indexOf(marker);
    if (idx >= 0){
      path = path.substring(idx + marker.length);
    }

    // 将 docs 拷贝后的路径映射回真实项目源路径
    // 示例：driver/Titan_driver_hyperram/README_zh.html -> project/Titan_driver_hyperram/README_zh.md
    var projectsDir = (window.versionInfo && window.versionInfo.projectsDir) ? window.versionInfo.projectsDir : '';
    var copyFiles = (window.versionInfo && window.versionInfo.copyFiles) ? window.versionInfo.copyFiles : [];
    var mapped = path;
    // 剥离 docs 分类前缀（start/basic/driver/component/...）后取剩余相对路径
    var firstSlash = mapped.indexOf('/');
    if (firstSlash >= 0) {
      mapped = mapped.substring(firstSlash + 1);
    }
    // 去掉 .html 后缀
    var htmlName = mapped.split('/').pop();
    var baseName = htmlName.replace(/\.html$/i, '');
    // 依据 copy_files 中的候选名推断源文件名优先级（README_zh.md、README.md 等）
    // 回退规则：
    // 1) 如果 baseName 匹配 README_zh/README 等，按列表顺序尝试；
    // 2) 否则将 .html 改为 .md
    var candidates = [];
    if (/^readme(_zh)?$/i.test(baseName)) {
      var normalized = copyFiles.map(function(n){ return String(n || '').toLowerCase(); });
      // 根据当前页面文件名匹配对应的源文件
      var targetFile = '';
      if (baseName.toLowerCase() === 'readme_zh') {
        targetFile = 'README_zh.md';
      } else if (baseName.toLowerCase() === 'readme') {
        targetFile = 'README.md';
      }
      
      // 如果找到匹配的目标文件且存在于 copyFiles 中，优先使用
      if (targetFile && normalized.indexOf(targetFile.toLowerCase()) >= 0) {
        candidates.push(targetFile);
      } else {
        // 回退到原有逻辑：常见文件名优先，保持原始大小写
        ['README_zh.md','README.md'].forEach(function(name){ 
          var lowerName = name.toLowerCase();
          if (normalized.indexOf(lowerName) >= 0 && candidates.indexOf(name) < 0) candidates.push(name); 
        });
      }
      
      // 追加其余列表中文件名（保持原始大小写）
      copyFiles.forEach(function(n){ 
        var fileName = String(n || '');
        var ln = fileName.toLowerCase(); 
        if (candidates.map(function(c){return c.toLowerCase();}).indexOf(ln) < 0) candidates.push(fileName); 
      });
    }
    if (candidates.length === 0) {
      candidates = [baseName + '.md'];
    }
    // 组装目录 + 源文件名
    var dir = mapped.substring(0, mapped.lastIndexOf('/'));
    mapped = dir + '/' + candidates[0];
    // 拼接为仓库中的项目路径
    if (projectsDir) {
      mapped = projectsDir.replace(/\/+$/,'') + '/' + mapped.replace(/^\//,'');
    }
    
    // 修复：正确构建 GitHub 编辑链接
    // 从 base URL 中提取 user 和 repo 信息
    // base 格式应该是类似 "https://github.com/user/repo/" 或 "github.com/user/repo/"
    var match = base.match(/github\.com\/([^\/]+)\/([^\/]+)\/?/i);
    if (!match) {
      console.warn('无法从 base URL 中解析 GitHub 用户名和仓库名:', base);
      return '';
    }
    
    var user = match[1];
    var repo = match[2];
    
    // 构建正确的 GitHub 编辑 URL 格式
    // https://github.com/$user/$repo/edit/$branch/$file_path
    var editUrl = 'https://github.com/' + user + '/' + repo + '/edit/' + branch + '/' + mapped;
    
    return editUrl;
  }

  function insertButton(){
    // 首先检查是否是可编辑的文档页面
    if (!isEditablePage()) return;
    
    var url = buildEditUrl();
    if (!url) return;

    // 检测当前页面语言
    var currentLang = detectCurrentLanguage();
    var buttonText = currentLang === 'zh' ? '在GitHub上编辑' : 'Edit on GitHub';
    var buttonTitle = currentLang === 'zh' ? '在GitHub上编辑此页面' : 'Edit this page on GitHub';

    // 优先插入到面包屑，保持与导航同高；使用带边框+图标样式
    var breadcrumbs = document.querySelector('.wy-breadcrumbs');
    if (breadcrumbs) {
      var oldAside = breadcrumbs.querySelector('.wy-breadcrumbs-aside');
      if (oldAside && oldAside.parentNode) oldAside.parentNode.removeChild(oldAside);
      var li = document.createElement('li');
      li.className = 'wy-breadcrumbs-aside';
      var a = document.createElement('a');
      a.href = url;
      a.target = '_blank';
      a.rel = 'noopener';
      a.className = 'sdk-edit-on-github__btn';
      a.innerHTML = '<svg class="sdk-edit-on-github__icon" viewBox="0 0 16 16" width="14" height="14" aria-hidden="true" focusable="false"><path fill="currentColor" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0 0 16 8c0-4.42-3.58-8-8-8Z"></path></svg><span>' + buttonText + '</span>';
      a.title = buttonTitle;
      li.appendChild(a);
      breadcrumbs.appendChild(li);
      return;
    }

    // 回退：插入到正文容器顶部右侧
    var content = document.querySelector('.wy-nav-content');
    if (!content) return;
    if (document.querySelector('.sdk-edit-on-github')) return;
    var container = document.createElement('div');
    container.className = 'sdk-edit-on-github';
    var a2 = document.createElement('a');
    a2.className = 'sdk-edit-on-github__btn';
    a2.target = '_blank';
    a2.rel = 'noopener';
    a2.href = url;
    a2.innerHTML = '<svg class="sdk-edit-on-github__icon" viewBox="0 0 16 16" width="14" height="14" aria-hidden="true" focusable="false"><path fill="currentColor" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0 0 16 8c0-4.42-3.58-8-8-8Z"></path></svg><span>' + buttonText + '</span>';
    a2.title = buttonTitle;
    container.appendChild(a2);
    var first = content.firstElementChild;
    if (first) content.insertBefore(container, first); else content.appendChild(container);
  }

  function init(){
    injectMetaFromContext();
    if (document.readyState === 'loading'){
      document.addEventListener('DOMContentLoaded', insertButton);
    } else {
      insertButton();
    }
  }

  init();
})();