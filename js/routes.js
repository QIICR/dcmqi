define([], function () {

  angular.module('app.routes', [])
    .config(function($routeProvider) {
      $routeProvider
        .when('/home', {
          templateUrl: 'home.html',
          controller: 'MainController'
        })
        .when('/seg', {
          templateUrl: 'seg.html',
          controller: 'SegmentationMetaCreatorController'
        })
        .when('/pmap', {
          templateUrl: 'pmap.html',
          controller: 'ParametricMapMetaCreatorController'
        })
        .when('/validators', {
          templateUrl: 'validators.html',
          controller: 'JSONValidatorController'
        })
        .otherwise({
          redirectTo: '/home'
        });
    });

});
