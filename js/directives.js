define(['controllers'], function () {

  function createNewAutoCompleteSelector(controller) {
    return {
      templateUrl: 'custom-autocomplete.html',
      restrict: 'EA',
      scope: {
        required: "=",
        segmentNumber: "@"
      },
      controller: controller
    };
  }

  var app = angular.module('app.directives', ['app.controllers']);

  app.directive('anatomicRegionSelector', function () {
    return createNewAutoCompleteSelector("AnatomicRegionController");
  });

  app.directive('anatomicRegionModifierSelector', function () {
    return createNewAutoCompleteSelector("AnatomicRegionModifierController");
  });

  app.directive('parametricMapQuantitySelector', function () {
    return createNewAutoCompleteSelector("ParametricMapQuantityCodeController");
  });

  app.directive('parametricMapMeasurementUnitSelector', function () {
    return createNewAutoCompleteSelector("ParametricMapMeasurementUnitsCodeController");
  });

  app.directive('segmentedPropertyCategorySelector', function () {
    return createNewAutoCompleteSelector("SegmentedPropertyCategoryCodeController");
  });

  app.directive('segmentedPropertyTypeSelector', function () {
    return createNewAutoCompleteSelector("SegmentedPropertyTypeController");
  });

  app.directive('segmentedPropertyTypeModifierSelector', function () {
    return createNewAutoCompleteSelector("SegmentedPropertyTypeModifierController");
  });

  app.directive("nonExistentLabel", function () {
    return {
      restrict: "A",

      require: "ngModel",

      link: function (scope, element, attributes, ngModel) {
        ngModel.$validators.nonExistentLabel = function (modelValue) {
          var exists = false;
          angular.forEach(scope.segments, function (value, key) {
            if (value.labelID == modelValue && value != scope.segment) {
              exists = true;
            }
          });
          return !exists;
        }
      }
    };
  });


  app.directive('resize', function ($window) {
    return {
      link: function postLink(scope, elem, attrs) {

        scope.onResizeFunction = function (element) {
          var toolbar = document.getElementById('toolbar');
          element.windowHeight = $window.innerHeight - toolbar.clientHeight;
          var newHeight = element.windowHeight - $(toolbar).height() / 2;
          var childrenHeight = 0;
          angular.forEach($(element).parent().children(), function (child, key) {
            if (child.id != $(element)[0].id)
              childrenHeight += $(child).height();
          });
          newHeight -= childrenHeight;
          $(element).height(newHeight);
        };

        scope.onResizeFunction(elem);

        angular.element($window).bind('resize', function () {
          scope.onResizeFunction(elem);
          scope.$apply();
        });
      }
    };
  });

});