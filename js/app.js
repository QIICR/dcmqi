define(['directives', 'routes', 'services'], function () {

  angular.module('app', [
    'ngRoute',
    'ngMaterial',
    'ngMessages',
    'ngMdIcons',
    'vAccordion',
    'ngAnimate',
    'xml',
    'ngclipboard',
    'mdColorPicker',
    'download',
    'ui.ace',
    'app.directives',
    'app.controllers',
    'app.routes',
    'app.services'])
    .config(function ($httpProvider) {
    $httpProvider.interceptors.push('xmlHttpInterceptor');
  })
    .config(function($mdThemingProvider) {
    $mdThemingProvider.theme('default')
      .primaryPalette('green')
      .accentPalette('red');
  });

});
