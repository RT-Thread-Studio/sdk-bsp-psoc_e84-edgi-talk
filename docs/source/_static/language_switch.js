/**
 * 中英文切换功能
 * 支持导航栏和正文内容的语言切换
 */
(function() {
    'use strict';
    
    // 语言配置
    const LANGUAGE_CONFIG = {
        chinese: {
            code: 'zh',
            name: '中文',
            suffix: '_zh',
            selector: '[data-lang="zh"]'
        },
        english: {
            code: 'en', 
            name: 'English',
            suffix: '',
            selector: '[data-lang="en"]'
        }
    };
    
    // 当前语言状态
    let currentLanguage = 'chinese';
    let isInitialized = false;
    
    /**
     * 检测当前页面的语言
     */
    function detectCurrentLanguage() {
        const htmlLang = document.documentElement.getAttribute('lang');
        const currentPath = window.location.pathname;
        if ((htmlLang === 'zh-CN') && (currentPath.endsWith('_zh.html'))) return 'chinese';
        if ((htmlLang === 'en') && (currentPath.endsWith('.html'))) return 'english';
        
        return 'english';
    }
    
    /**
     * 初始化语言切换UI（动态创建）
     */
    function createLanguageSwitch() {
        // 检查是否已经存在
        const existingSwitch = document.querySelector('.language-switch');
        if (existingSwitch) {
            console.log('语言切换器已存在');
            return;
        }
        
        // 查找侧边栏容器（使用 .wy-nav-side，确保位于侧栏根部而非滚动容器内）
        const sidebar = document.querySelector('.wy-nav-side');
        if (!sidebar) {
            console.warn('未找到侧边栏容器');
            return;
        }
        
        // 创建语言切换UI - "中文 | English" 格式
        const languageSwitchHTML = `
            <div class="language-switch-container">
                <div class="language-switch" id="language-switch">
                    <div class="language-switch__container">
                        <button class="language-switch__option" data-lang="chinese" aria-label="切换到中文">
                            中文
                        </button>
                        <span class="language-switch__separator">|</span>
                        <button class="language-switch__option" data-lang="english" aria-label="Switch to English">
                            English
                        </button>
                    </div>
                </div>
            </div>
        `;
        
        // 将语言切换器添加到侧边栏底部（作为 .wy-nav-side 的直接子元素）
        sidebar.insertAdjacentHTML('beforeend', languageSwitchHTML);
        console.log('语言切换器已动态创建');
    }
    
    /**
     * 设置语言切换器事件
     */
    function setupLanguageSwitchEvents() {
        const languageSwitch = document.getElementById('language-switch');
        if (!languageSwitch) return;
        
        const options = languageSwitch.querySelectorAll('.language-switch__option');
        const slider = languageSwitch.querySelector('.language-switch__slider');
        
        options.forEach(option => {
            option.addEventListener('click', function(e) {
                e.preventDefault();
                const targetLang = this.getAttribute('data-lang');
                if (targetLang !== currentLanguage) {
                    switchLanguage(targetLang);
                }
            });
            
            // 键盘支持
            option.addEventListener('keydown', function(e) {
                if (e.key === 'Enter' || e.key === ' ') {
                    e.preventDefault();
                    this.click();
                }
            });
        });
        
        // 更新当前语言状态
        updateLanguageSwitchState();
    }
    
    /**
     * 更新语言切换器状态
     */
    function updateLanguageSwitchState() {
        const languageSwitch = document.getElementById('language-switch');
        if (!languageSwitch) return;
        
        const options = languageSwitch.querySelectorAll('.language-switch__option');
        
        options.forEach(option => {
            const lang = option.getAttribute('data-lang');
            if (lang === currentLanguage) {
                option.classList.add('active');
                option.setAttribute('aria-pressed', 'true');
            } else {
                option.classList.remove('active');
                option.setAttribute('aria-pressed', 'false');
            }
        });
    }
    
    /**
     * 切换语言
     */
    function switchLanguage(targetLang) {
        if (targetLang === currentLanguage) return;
        
        console.log(`切换语言: ${currentLanguage} -> ${targetLang}`);
        
        // 添加加载状态
        const languageSwitch = document.getElementById('language-switch');
        if (languageSwitch) {
            languageSwitch.classList.add('loading');
        }
        
        // 获取当前页面信息
        const currentPath = window.location.pathname;
        const currentUrl = window.location.href;
        
        // 构建目标URL
        let targetUrl = '';
        
        if (targetLang === 'chinese') {
            // 切换到中文版本
            if (currentPath.includes('_zh.html')) {
                // 如果已经是中文页面，不需要切换
                targetUrl = currentUrl;
            } else if (currentPath.includes('.html') && !currentPath.includes('_zh.html')) {
                // 如果当前是英文页面，切换到中文页面
                targetUrl = currentPath.replace('.html', '_zh.html');
            } else if (currentPath.endsWith('/') || currentPath.endsWith('/index.html')) {
                // 如果当前是根目录或英文版目录页面，切换到中文版目录页面
                targetUrl = currentPath.replace('/index.html', '/index_zh.html').replace(/\/$/, '/index_zh.html');
            } else {
                // 其他情况，尝试构建中文版本URL
                targetUrl = currentPath + (currentPath.endsWith('/') ? '' : '/') + 'index_zh.html';
            }
        } else {
            // 切换到英文版本
            if (currentPath.includes('_zh.html')) {
                // 如果当前是中文页面，切换到英文页面
                targetUrl = currentPath.replace('_zh.html', '.html');
            } else if (currentPath.includes('.html') && !currentPath.includes('_zh.html')) {
                // 如果当前是英文页面，保持英文版本
                targetUrl = currentUrl;
            } else if (currentPath.endsWith('/') || currentPath.endsWith('/index_zh.html')) {
                // 如果当前是根目录或中文版目录页面，切换到英文版目录页面
                targetUrl = currentPath.replace('/index_zh.html', '/index.html').replace(/\/$/, '/index.html');
            } else {
                // 其他情况，尝试构建英文版本URL
                targetUrl = currentPath + (currentPath.endsWith('/') ? '' : '/') + 'index.html';
            }
        }
        
        // 如果目标URL与当前URL相同，只更新UI状态
        if (targetUrl === currentUrl || targetUrl === window.location.href) {
            currentLanguage = targetLang;
            updateLanguageSwitchState();
            if (languageSwitch) {
                languageSwitch.classList.remove('loading');
            }
            return;
        }
        
        // 跳转到目标URL
        console.log(`跳转到: ${targetUrl}`);
        window.location.href = targetUrl;
    }
    
    /**
     * 更新导航栏语言显示
     */
    function updateNavigationLanguage() {
        const navLinks = document.querySelectorAll('.wy-menu a[href*="README"]');
        
        navLinks.forEach(link => {
            const href = link.getAttribute('href');
            const text = link.textContent.trim();
            
            if (currentLanguage === 'chinese') {
                // 显示中文版本，隐藏英文版本
                if (href.includes('_zh.html') || text.includes('(中文)')) {
                    link.style.display = '';
                    link.closest('li').style.display = '';
                } else if (href.includes('.html') && !href.includes('_zh.html') && text.includes('(English)')) {
                    link.style.display = 'none';
                    link.closest('li').style.display = 'none';
                }
            } else {
                // 显示英文版本，隐藏中文版本
                if (href.includes('_zh.html') || text.includes('(中文)')) {
                    link.style.display = 'none';
                    link.closest('li').style.display = 'none';
                } else if (href.includes('.html') && !href.includes('_zh.html') && text.includes('(English)')) {
                    link.style.display = '';
                    link.closest('li').style.display = '';
                }
            }
        });
    }
    
    /**
     * 初始化语言切换功能
     */
    function init() {
        if (isInitialized) return;
        
        console.log('初始化语言切换功能...');
        
        // 检测当前语言
        currentLanguage = detectCurrentLanguage();
        console.log('检测到的当前语言:', currentLanguage);
        
        // 等待DOM加载完成
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', function() {
                setTimeout(initLanguageSwitch, 100);
            });
        } else {
            setTimeout(initLanguageSwitch, 100);
        }
        
        isInitialized = true;
    }
    
    /**
     * 初始化语言切换器
     */
    function initLanguageSwitch() {
        // 如果模板已经渲染在 .wy-side-scroll 内部，则搬移到 .wy-nav-side 下
        const existingContainer = document.querySelector('.wy-side-scroll > .language-switch-container');
        const sideRoot = document.querySelector('.wy-nav-side');
        if (existingContainer && sideRoot) {
            sideRoot.appendChild(existingContainer);
        }

        createLanguageSwitch();
        setupLanguageSwitchEvents();
        updateNavigationLanguage();
        
        // 如果侧边栏是动态加载的，等待一下再尝试
        setTimeout(() => {
            if (!document.querySelector('.language-switch')) {
                console.warn('语言切换器仍未找到，请检查模板配置');
            }
        }, 1000);
    }
    
    // 启动
    init();
    
    // 导出到全局作用域
    window.LanguageSwitch = {
        switchLanguage: switchLanguage,
        getCurrentLanguage: () => currentLanguage,
        updateNavigation: updateNavigationLanguage
    };
    
})();
