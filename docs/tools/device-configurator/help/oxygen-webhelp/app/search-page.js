/**
 * Load the libraries for the Search page.
 */
define(["require", "config"], function() {
    require(['options'], function(options){
        const jsModules = [
            'polyfill', 
            'menu',
            'webhelp',
            'codeblock',
            'wh-sticky',
            'top-menu',
            'template-module-loader'
        ];
        if(!options.getBoolean("webhelp.custom.search.engine.enabled")) {
            require(['search'], function() {
                jsModules.push('searchAutocomplete');
                require(jsModules);
            });
        } else {
            require(jsModules);
        }
    });
});