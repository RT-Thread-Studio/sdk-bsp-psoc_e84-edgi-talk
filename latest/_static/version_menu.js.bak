// Read the Docs 风格的版本菜单功能
(function() {
    'use strict';
    
    // 版本配置 - 将从版本配置文件获取
    let VERSION_CONFIG = {
        current: 'main',
        versions: {}
    };
    
    // 从版本配置文件获取版本信息
    async function fetchVersionInfo() {
        try {
            // 智能检测当前版本
            const detectedVersion = detectCurrentVersion();
            console.log('检测到的当前版本:', detectedVersion);
            
            // 检查是否为本地文件系统
            const isLocalFileSystem = window.location.protocol === 'file:';
            console.log('是否为本地文件系统:', isLocalFileSystem);
            
            let config = null;
            let configPath = '';
            
            console.log('本地文件系统，使用嵌入的版本配置');
            const embeddedConfig = getEmbeddedVersionConfig();
            if (embeddedConfig) {
                config = embeddedConfig;
                configPath = 'embedded';
                console.log('使用嵌入的版本配置');
            } else {
                console.log('嵌入配置不可用，尝试其他方法');
            }
            
            // 如果仍然没有配置，使用嵌入的配置作为兜底
            if (!config) {
                console.log('尝试使用嵌入的版本配置作为兜底');
                const embeddedConfig = getEmbeddedVersionConfig();
                if (embeddedConfig) {
                    config = embeddedConfig;
                    configPath = 'embedded_fallback';
                    console.log('使用嵌入的版本配置作为兜底');
                }
            }
            
            if (!config) {
                throw new Error('无法找到版本配置文件');
            }
            
            // 确定当前版本
            const currentPath = window.location.pathname;
            let currentVersion = config.default_version || 'main';
            
            // 从URL路径判断当前版本
            for (const version of config.versions) {
                if (currentPath.includes(`/${version.url_path}/`)) {
                    currentVersion = version.name;
                    break;
                }
            }
            
            // 构建版本配置对象
            const versions = {};
            for (const version of config.versions) {
                versions[version.name] = {
                    display_name: version.display_name,
                    url_path: version.url_path,
                    description: version.description
                };
            }
            
            VERSION_CONFIG = {
                current: currentVersion,
                versions: versions
            };
            
            console.log('版本信息已更新:', VERSION_CONFIG);
            console.log('配置文件路径:', configPath);
            
            // 重新创建版本菜单
            createVersionMenu();
            
        } catch (error) {
            console.warn('无法获取版本信息，使用默认配置:', error);
            
            // 使用默认配置
            VERSION_CONFIG = {
                current: 'main',
                versions: {
                    'main': {
                        display_name: '最新版本',
                        url_path: 'latest',
                        description: '最新开发版本'
                    }
                }
            };
            
            createVersionMenu();
        }
    }
    
    // 智能检测当前版本
    function detectCurrentVersion() {
        const pathname = window.location.pathname;
        console.log('当前路径:', pathname);
        
        // 检查是否在版本子目录中
        const versionMatch = pathname.match(/\/(v\d+\.\d+|latest)\//);
        if (versionMatch) {
            const detectedVersion = versionMatch[1];
            console.log('检测到版本目录:', detectedVersion);
            return detectedVersion;
        }
        
        // 检查URL参数
        const urlParams = new URLSearchParams(window.location.search);
        const versionParam = urlParams.get('version');
        if (versionParam) {
            console.log('从URL参数检测到版本:', versionParam);
            return versionParam;
        }
        
        console.log('未检测到版本信息，使用默认路径');
        return null;
    }
    
    // 创建版本菜单
    function createVersionMenu() {
        // 查找侧边栏 - 改进查找逻辑
        let sidebar = document.querySelector('.wy-nav-side');
        if (!sidebar) {
            // 尝试其他可能的选择器
            sidebar = document.querySelector('nav[data-toggle="wy-nav-shift"]');
        }
        if (!sidebar) {
            sidebar = document.querySelector('.wy-side-scroll');
        }
        if (!sidebar) {
            console.warn('找不到侧边栏元素，尝试在body中创建版本菜单');
            sidebar = document.body;
        }
        
        // 检查是否已经存在版本菜单
        if (document.querySelector('.rtd-version-menu')) {
            console.log('版本菜单已存在，跳过创建');
            return;
        }
        
        // 计算最长版本名称的宽度
        const versionNames = Object.values(VERSION_CONFIG.versions).map(v => v.display_name);
        const maxLength = Math.max(...versionNames.map(name => name.length));
        const minWidth = Math.max(80, maxLength * 5 + 20);
        
        // 创建版本菜单容器
        const versionMenu = document.createElement('div');
        versionMenu.className = 'rtd-version-menu';
        versionMenu.style.minWidth = minWidth + 'px';
        versionMenu.innerHTML = `
            <button class="rtd-version-menu__button" type="button" aria-haspopup="true" aria-expanded="false">
                <span class="rtd-version-menu__current">${VERSION_CONFIG.versions[VERSION_CONFIG.current]?.display_name || '版本'}</span>
            </button>
            <div class="rtd-version-menu__dropdown" role="menu">
                ${Object.entries(VERSION_CONFIG.versions).map(([version, versionInfo]) => `
                    <a class="rtd-version-menu__item ${version === VERSION_CONFIG.current ? 'active' : ''}" 
                       href="#" data-version="${version}" role="menuitem">
                        ${versionInfo.display_name}
                    </a>
                `).join('')}
            </div>
        `;
        
        // 尝试不同的插入位置
        let inserted = false;
        
        // 1. 尝试插入到项目标题下方
        const projectTitle = sidebar.querySelector('a.icon.icon-home');
        if (projectTitle && projectTitle.parentNode) {
            projectTitle.parentNode.insertBefore(versionMenu, projectTitle.nextSibling);
            inserted = true;
            console.log('版本菜单已插入到项目标题下方');
        }
        
        // 2. 尝试插入到搜索框下方
        if (!inserted) {
            const searchForm = sidebar.querySelector('#rtd-search-form');
            if (searchForm && searchForm.parentNode) {
                searchForm.parentNode.insertBefore(versionMenu, searchForm.nextSibling);
                inserted = true;
                console.log('版本菜单已插入到搜索框下方');
            }
        }
        
        // 3. 尝试插入到侧边栏顶部
        if (!inserted) {
            const firstChild = sidebar.firstChild;
            if (firstChild) {
                sidebar.insertBefore(versionMenu, firstChild);
                inserted = true;
                console.log('版本菜单已插入到侧边栏顶部');
            }
        }
        
        // 4. 如果都失败了，直接添加到侧边栏
        if (!inserted) {
            sidebar.appendChild(versionMenu);
            console.log('版本菜单已添加到侧边栏');
        }
        
        // 添加事件监听器
        setupVersionMenuEvents(versionMenu);
    }
    
    // 设置版本菜单事件
    function setupVersionMenuEvents(versionMenu) {
        const button = versionMenu.querySelector('.rtd-version-menu__button');
        const dropdown = versionMenu.querySelector('.rtd-version-menu__dropdown');
        
        if (!button || !dropdown) {
            console.warn('找不到版本菜单按钮或下拉菜单');
            return;
        }
        
        // 按钮点击事件
        button.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            
            const isExpanded = this.getAttribute('aria-expanded') === 'true';
            this.setAttribute('aria-expanded', !isExpanded);
            dropdown.classList.toggle('show');
            
            console.log('版本菜单切换:', !isExpanded);
        });
        
        // 版本选择事件
        dropdown.addEventListener('click', function(e) {
            if (e.target.classList.contains('rtd-version-menu__item')) {
                e.preventDefault();
                e.stopPropagation();
                
                const version = e.target.getAttribute('data-version');
                const versionInfo = VERSION_CONFIG.versions[version];
                if (!versionInfo) {
                    console.error('找不到版本信息:', version);
                    return;
                }
                
                const versionName = versionInfo.display_name;
                
                // 更新当前版本显示
                const currentSpan = button.querySelector('.rtd-version-menu__current');
                if (currentSpan) {
                    currentSpan.textContent = versionName;
                }
                
                // 更新活动状态
                dropdown.querySelectorAll('.rtd-version-menu__item').forEach(item => {
                    item.classList.remove('active');
                });
                e.target.classList.add('active');
                
                // 关闭下拉菜单
                button.setAttribute('aria-expanded', 'false');
                dropdown.classList.remove('show');
                
                // 版本切换逻辑
                handleVersionChange(version, versionName);
            }
        });
        
        // 点击外部关闭下拉菜单
        document.addEventListener('click', function(e) {
            if (!versionMenu.contains(e.target)) {
                button.setAttribute('aria-expanded', 'false');
                dropdown.classList.remove('show');
            }
        });
        
        // 键盘导航支持
        button.addEventListener('keydown', function(e) {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                this.click();
            }
        });
        
        dropdown.addEventListener('keydown', function(e) {
            const items = Array.from(this.querySelectorAll('.rtd-version-menu__item'));
            const currentIndex = items.indexOf(document.activeElement);
            
            switch (e.key) {
                case 'ArrowDown':
                    e.preventDefault();
                    const nextIndex = (currentIndex + 1) % items.length;
                    items[nextIndex].focus();
                    break;
                case 'ArrowUp':
                    e.preventDefault();
                    const prevIndex = (currentIndex - 1 + items.length) % items.length;
                    items[prevIndex].focus();
                    break;
                case 'Escape':
                    e.preventDefault();
                    button.setAttribute('aria-expanded', 'false');
                    dropdown.classList.remove('show');
                    button.focus();
                    break;
            }
        });
    }
    
    // 处理版本切换
    function handleVersionChange(version, versionName) {
        console.log('切换到版本:', version, versionName);
        
        // 获取当前页面的相对路径
        const currentPath = window.location.pathname;
        const baseUrl = window.location.origin;
        
        // 检查是否为本地开发环境
        const isLocalDev = window.location.hostname === 'localhost' || 
                          window.location.hostname === '127.0.0.1' || 
                          window.location.protocol === 'file:';
        
        // 获取目标版本的URL路径
        const targetVersionInfo = VERSION_CONFIG.versions[version];
        if (!targetVersionInfo) {
            console.error('找不到目标版本信息:', version);
            return;
        }
        
        const targetUrlPath = targetVersionInfo.url_path;
        
        // 简化URL构建逻辑
        let newUrl;
        
        if (isLocalDev) {
            // 本地开发环境：简化路径处理
            const currentUrlPath = getCurrentVersionPath();
            if (currentUrlPath) {
                // 替换版本目录
                const newPath = currentPath.replace(`/${currentUrlPath}/`, `/${targetUrlPath}/`);
                newUrl = `file://${newPath}`;
            } else {
                // 如果不在版本目录中，构建到版本目录的路径
                const fileName = currentPath.split('/').pop();
                const directory = currentPath.substring(0, currentPath.lastIndexOf('/'));
                newUrl = `file://${directory}/${targetUrlPath}/${fileName}`;
            }
        } else {
            // 生产环境：简化路径处理
            const relativePath = getRelativePathFromCurrentVersion();
            const repoName = getRepoNameFromPath() || 'sdk-bsp-rzn2l-etherkit';
            
            newUrl = `${baseUrl}/${repoName}/${targetUrlPath}/${relativePath}`;
            
            // 确保URL以 / 结尾（如果是根路径）
            if (!relativePath || relativePath === '/') {
                newUrl = newUrl.replace(/\/$/, '') + '/';
            }
        }
        
        console.log('当前路径:', currentPath);
        console.log('是否为本地开发:', isLocalDev);
        console.log('跳转到:', newUrl);
        
        // 跳转到新版本
        window.location.href = newUrl;
        
        // 发送自定义事件
        const event = new CustomEvent('version-changed', {
            detail: {
                version: version,
                versionName: versionName,
                newUrl: newUrl
            }
        });
        document.dispatchEvent(event);
    }
    
    // 获取当前版本路径
    function getCurrentVersionPath() {
        const currentPath = window.location.pathname;
        for (const [verName, verInfo] of Object.entries(VERSION_CONFIG.versions)) {
            if (currentPath.includes(`/${verInfo.url_path}/`)) {
                return verInfo.url_path;
            }
        }
        return null;
    }
    
    // 从当前版本路径中提取相对路径
    function getRelativePathFromCurrentVersion() {
        const currentPath = window.location.pathname;
        for (const [verName, verInfo] of Object.entries(VERSION_CONFIG.versions)) {
            if (currentPath.includes(`/${verInfo.url_path}/`)) {
                const parts = currentPath.split(`/${verInfo.url_path}/`);
                return parts.length > 1 ? parts[1] : '';
            }
        }
        return '';
    }
    
    // 从路径中提取仓库名称
    function getRepoNameFromPath() {
        const currentPath = window.location.pathname;
        const pathParts = currentPath.split('/');
        
        for (let i = 0; i < pathParts.length - 1; i++) {
            if (pathParts[i] && pathParts[i + 1]) {
                const isVersionPath = Object.values(VERSION_CONFIG.versions).some(v => v.url_path === pathParts[i + 1]);
                if (isVersionPath) {
                    return pathParts[i];
                }
            }
        }
        return null;
    }
    
    // 初始化版本菜单
    function init() {
        console.log('初始化版本菜单...');
        
        // 等待 DOM 加载完成
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', fetchVersionInfo);
        } else {
            fetchVersionInfo();
        }
        
        // 如果侧边栏是动态加载的，等待一下再尝试创建
        setTimeout(() => {
            if (!document.querySelector('.rtd-version-menu')) {
                createVersionMenu();
            }
        }, 1000);
        
        setTimeout(() => {
            if (!document.querySelector('.rtd-version-menu')) {
                createVersionMenu();
            }
        }, 2000);
        
        setTimeout(() => {
            if (!document.querySelector('.rtd-version-menu')) {
                createVersionMenu();
            }
        }, 3000);
    }
    
    // 启动
    init();
})(); 
