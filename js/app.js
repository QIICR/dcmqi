(function(angular) {

  var rev = '1f7f99077892ae432915b6f0fe3a6cdc57b05e88';
  var webAssets = 'https://raw.githubusercontent.com/QIICR/dcmqi/'+rev+'/doc/';

  var anatomicRegionJSONPath = webAssets+'segContexts/AnatomicRegionAndModifier.json'; // fallback should be local
  var segmentationCategoryJSONPath = webAssets+'segContexts/SegmentationCategoryTypeModifierRGB.json'; // fallback should be local

  var app = angular.module('JSONSemanticsCreator', ['ngRoute', 'ngMaterial', 'ngMessages', 'ngMdIcons', 'ngAnimate',
                                                    'xml', 'ngclipboard', 'ui-notification', 'mdColorPicker']);

  app.config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });

  app.config(function($mdThemingProvider) {
    $mdThemingProvider.theme('default')
      .primaryPalette('green')
      .accentPalette('red');
  });

  app.config(function(NotificationProvider) {
      NotificationProvider.setOptions({
        delay: 5000,
        startTop: 10,
        startRight: 10,
        positionX: 'right',
        positionY: 'bottom'
      });
    });

  app.config(function($routeProvider) {
    $routeProvider
      .when('/home', {
        templateUrl: 'home.html',
        controller: 'JSONSemanticsCreatorMainController'
      })
      .when('/seg', {
        templateUrl: 'seg.html',
        controller: 'JSONSemanticsCreatorController'
      })
      .otherwise({
        redirectTo: '/home'
      });
  });

  app.controller('JSONSemanticsCreatorMainController', ['$scope',
    function($scope) {
      $scope.headlineText = "DCMQI Meta Information Generators";
      $scope.toolTipDelay = 500;
  }]);

  app.controller('JSONSemanticsCreatorController',
                 ['$scope', '$rootScope', '$log', '$mdDialog', '$timeout', 'Notification',
    function($scope, $rootScope, $log, $mdDialog, $timeout, Notification) {

      var self = this;
      self.segmentedPropertyCategory = null;
      self.segmentedPropertyType = null;
      self.segmentedPropertyTypeModifier = null;
      self.anatomicRegion = null;
      self.anatomicRegionModifier = null;

      $scope.submitForm = function(isValid) {
        if (isValid) {
          self.createJSONOutput();
        } else {
          self.showErrors();
        }
      };

      $scope.resetForm = function() {
        $scope.seriesAttributes = angular.extend({}, seriesAttributesDefaults);
        $scope.segmentAttributes.LabelID = 1;
        $scope.segments.length = 0;
        $scope.segments.push(angular.extend({}, $scope.segmentAttributes));
        $scope.segments[0].RecommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.output = undefined;
      };

      var seriesAttributesDefaults = {
        ReaderID : "Reader1",
        SessionID : "Session1",
        TimePointID : "1",
        SeriesDescription : "Segmentation",
        SeriesNumber : 300,
        InstanceNumber : 1,
        BodyPartExamined :  ""
      };

      var colorPickerDefaultOptions = {
        clickOutsideToClose: true,
        openOnInput: true,
        mdColorAlphaChannel: false,
        mdColorClearButton: false,
        mdColorSliders: false,
        mdColorHistory: false,
        mdColorGenericPalette: false,
        mdColorMaterialPalette:false,
        mdColorHex: false,
        mdColorHsl: false
      };

      var defaultRecommendedDisplayValue = {
        color: '',
        backgroundOptions: angular.extend({}, colorPickerDefaultOptions)
      };

      $scope.seriesAttributes = angular.extend({}, seriesAttributesDefaults);

      $scope.segmentAttributes = {
        LabelID: 1,
        SegmentDescription: "",
        AnatomicRegion: null,
        AnatomicRegionModifier: null,
        SegmentedPropertyCategoryCode: null,
        SegmentedPropertyType: null,
        SegmentedPropertyTypeModifier: null,
        RecommendedDisplayRGBValue: angular.extend({}, defaultRecommendedDisplayValue)
      };


      var segment = angular.extend({}, $scope.segmentAttributes);
      $scope.segments = [segment];
      $scope.output = undefined;

      $scope.addSegment = function() {
        $scope.segmentAttributes.LabelID += 1;
        var segment = angular.extend({}, $scope.segmentAttributes);
        segment.RecommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.segments.push(segment);
        $scope.selectedIndex = $scope.segments.length-1;
      };

      $scope.removeSegment = function() {
        $scope.segments.splice($scope.selectedIndex, 1);
        if ($scope.selectedIndex-1 < 0)
          $scope.selectedIndex = 0;
        else
          $scope.selectedIndex -= 1;
        $scope.output = undefined;
      };

      $scope.previousSegment = function() {
        $scope.selectedIndex -= 1;
      };

      $scope.nextSegment = function() {
        $scope.selectedIndex += 1;
      };

      $scope.error = function(message) {
        Notification.error(message);
      };

      self.showErrors = function() {
        $scope.output = undefined;
        angular.forEach($scope.jsonForm.$error.required, function (error, key) {
          var elements = error.$name.split("_");
          var message = "[MISSING]: " + elements[0];
          if (elements[1] != undefined)
            message += " for segment with label id " + elements[1];
          $scope.error(message);
        });
      };

      $scope.segmentAlreadyExists = function(segment) {
        var exists = false;
        angular.forEach($scope.segments, function(value, key) {
          if(value.LabelID == segment.LabelID && value != segment) {
            exists = true;
          }
        });
        return exists;
      };

      self.createJSONOutput = function() {

        var seriesAttributes = {
          "ReaderID": $scope.seriesAttributes.ReaderID,
          "SessionID" : $scope.seriesAttributes.SessionID,
          "TimePointID" : $scope.seriesAttributes.TimePointID,
          "SeriesDescription" : $scope.seriesAttributes.SeriesDescription,
          "SeriesNumber" : $scope.seriesAttributes.SeriesNumber,
          "InstanceNumber" : $scope.seriesAttributes.InstanceNumber
        };

        if ($scope.seriesAttributes.BodyPartExamined.length > 0)
          seriesAttributes["BodyPartExamined"] = $scope.seriesAttributes.BodyPartExamined;

        var segmentAttributes = [];
        angular.forEach($scope.segments, function(value, key) {
          var attributes = {};
          attributes["LabelID"] = value.LabelID;
          if (value.SegmentDescription.length > 0)
            attributes["SegmentDescription"] = value.SegmentDescription;
          if (value.anatomicRegion)
            attributes["AnatomicRegionCodeSequence"] = getCodeSequenceAttributes(value.anatomicRegion);
          if (value.anatomicRegionModifier)
            attributes["AnatomicRegionModifierCodeSequence"] = getCodeSequenceAttributes(value.anatomicRegionModifier);
          if (value.segmentedPropertyCategory)
            attributes["SegmentedPropertyCategoryCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyCategory);
          if (value.segmentedPropertyType)
            attributes["SegmentedPropertyTypeCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyType);
          if (value.segmentedPropertyTypeModifier)
            attributes["SegmentedPropertyTypeModifierCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyTypeModifier);
          if (value.RecommendedDisplayRGBValue.color)
            attributes["RecommendedDisplayRGBValue"] = self.rgbToArray(value.RecommendedDisplayRGBValue.color);
          segmentAttributes.push(attributes);
        });

        var doc = {
          "seriesAttributes": seriesAttributes,
          "segmentAttributes": segmentAttributes
        };

        $scope.output = doc;
      };

      self.rgbToArray = function(str) {
        var rgb = str.replace("rgb(", "").replace(")", "").split(", ");
        return [rgb[0], rgb[1], rgb[2]];
      }

  }]);

  function getCodeSequenceAttributes(codeSequence) {
    if (codeSequence != null && codeSequence != undefined)
      return {"CodeValue":codeSequence.codeValue,
              "CodingSchemeDesignator":codeSequence.codingScheme,
              "CodeMeaning":codeSequence.codeMeaning}
  }

  app.controller('CodeSequenceBaseController',
    function($self, $scope, $rootScope, $http, $log, $timeout, $q) {
      var self = $self;

      self.simulateQuery = false;
      self.isDisabled    = false;

      self.querySearch   = querySearch;
      self.selectedItemChange = selectedItemChange;
      self.searchTextChange   = searchTextChange;
      self.selectionChangedEvent = "";

      self.getName = function() {
        return self.floatingLabel.replace(" ", "");
      };

      function querySearch (query) {
        var results = query ? self.mappedCodes.filter( createFilterFor(query) ) : self.mappedCodes,
          deferred;
        if (self.simulateQuery) {
          deferred = $q.defer();
          $timeout(function () { deferred.resolve( results ); }, Math.random() * 1000, false);
          return deferred.promise;
        } else {
          return results;
        }
      }

      function searchTextChange(text) {
        // $log.info('Text changed to ' + text);
      }

      function selectedItemChange(item) {
        // $log.info('Item changed to ' + JSON.stringify(item));
        $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
      }

      function createFilterFor(query) {
        var lowercaseQuery = angular.lowercase(query);
        return function filterFn(item) {
          return (item.value.indexOf(lowercaseQuery) != -1);
        };
      }

      self.codesList2codeMeaning = function(list) {
        if(Object.prototype.toString.call( list ) != '[object Array]' ) {
          list = [list];
        }
        list.sort(function(a,b) {return (a.codeMeaning > b.codeMeaning) ? 1 : ((b.codeMeaning > a.codeMeaning) ? -1 : 0);});
        return list.map(function (code) {
          return {
            value: code.codeMeaning.toLowerCase(),
            contextGroupName : code.contextGroupName,
            display: code.codeMeaning,
            object: code
          };
        })
      };
    }
  );

  app.controller('AnatomicRegionController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Anatomic Region";
    self.selectionChangedEvent = "AnatomicRegionSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.anatomicRegion = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $http.get(anatomicRegionJSONPath).success(function (data) {
      $scope.anatomicCodes = data.AnatomicCodes.AnatomicRegion;
      self.mappedCodes = self.codesList2codeMeaning($scope.anatomicCodes);
    });
  });


  app.controller('AnatomicRegionModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Anatomic Region Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "AnatomicRegionModifierSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.anatomicRegionModifier = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Modifier);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });

  app.controller('SegmentedPropertyCategoryCodeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.required = true;
    self.floatingLabel = "Segmented Category";
    self.selectionChangedEvent = "SegmentedPropertyCategorySelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyCategory = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $http.get(segmentationCategoryJSONPath).success(function (data) {
      $scope.segmentationCodes = data.SegmentationCodes.Category;
      self.mappedCodes = self.codesList2codeMeaning($scope.segmentationCodes);
    });
  });


  app.controller('SegmentedPropertyTypeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.required = true;
    self.floatingLabel = "Segmented Property Type";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyType = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
      if (self.selectedItem === null) {
        $scope.segment.RecommendedDisplayRGBValue.color = "";
        $scope.segment.hasRecommendedColor = false;
      }
      else if (self.selectedItem.object.recommendedDisplayRGBValue != undefined) {
        $scope.segment.hasRecommendedColor = true;
        var rgb = self.selectedItem.object.recommendedDisplayRGBValue;
        $scope.segment.RecommendedDisplayRGBValue.color = 'rgb('+rgb[0]+', '+rgb[1]+', '+rgb[2]+')';
      }
    };

    $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Type === undefined;
        if (data.item.object.Type === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Type);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });


  app.controller('SegmentedPropertyTypeModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Segmented Property Type Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeModifierSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyTypeModifier = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Modifier);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });


  app.directive('customAutocomplete', function() {
    return {
      templateUrl: 'custom-autocomplete.html'
    };
  });

  app.directive("nonExistentLabel", function() {
    return {
      restrict: "A",

      require: "ngModel",

      link: function(scope, element, attributes, ngModel) {
        ngModel.$validators.nonExistentLabel = function(modelValue) {
          var exists = false;
          angular.forEach(scope.segments, function(value, key) {
            if(value.LabelID == modelValue && value != scope.segment) {
              exists = true;
            }
          });
          return !exists;
        }
      }
    };
  });

})(window.angular);
