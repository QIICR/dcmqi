define(['ajv'], function (Ajv) {

  var user = 'qiicr';
  var rev = 'master';
  var webAssets = 'https://raw.githubusercontent.com/'+user+'/dcmqi/'+rev+'/doc/';

  var commonSchemaURL = webAssets + 'common-schema.json';
  var segSchemaURL = webAssets + 'seg-schema.json';

  var segSchemaID = 'https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/seg-schema.json'; // VERY IMPORTANT! OTHERWISE resolving fails

  var anatomicRegionJSONPath = webAssets+'segContexts/AnatomicRegionAndModifier.json'; // fallback should be local
  var segmentationCategoryJSONPath = webAssets+'segContexts/SegmentationCategoryTypeModifierRGB.json'; // fallback should be local

  var app = angular.module('JSONSemanticsCreator', ['ngRoute', 'ngMaterial', 'ngMessages', 'ngMdIcons', 'vAccordion',
                                                    'ngAnimate', 'xml', 'ngclipboard', 'mdColorPicker', 'download']);

  app.config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });

  app.config(function($mdThemingProvider) {
    $mdThemingProvider.theme('default')
      .primaryPalette('green')
      .accentPalette('red');
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
                 ['$scope', '$rootScope', '$http', '$log', '$mdToast', 'download',
    function($scope, $rootScope, $http, $log, $mdToast, download) {

      var self = this;
      self.segmentedPropertyCategory = null;
      self.segmentedPropertyType = null;
      self.segmentedPropertyTypeModifier = null;
      self.anatomicRegion = null;
      self.anatomicRegionModifier = null;
      var currentLabelID = 1;

      $scope.submitForm = function(isValid) {
        if (isValid) {
          self.createJSONOutput();
          hideToast();
        } else {
          self.showErrors();
        }
      };

      $scope.downloadFile = function() {
        download.fromData($scope.output, "text/json", $scope.seriesAttributes.ClinicalTrialSeriesID+".json");
      };

      $scope.validJSON = false;

      var ajv = new Ajv({
        useDefaults: true,
        allErrors: true,
        loadSchema: loadSchema });

      var schemaLoaded = false;
      var validate = undefined;
      var commonSchema = undefined;
      var segSchema = undefined;
      var segment = undefined;

      loadSchema(commonSchemaURL, function(err, body) {
        if (body != undefined) {
          commonSchema = body.data;
          ajv.addSchema(commonSchema);
        }
        loadSchema(segSchemaURL, function(err, body){
          if (body != undefined) {
            segSchema = body.data;
            ajv.addSchema(segSchema);
            schemaLoaded = true;
          }
          $scope.resetForm();
        });
      });

      function loadDefaultSeriesAttributes() {
        var doc = {
          "seriesAttributes": {}
        };
        if (schemaLoaded) {
          validate = ajv.compile({$ref: segSchemaID});
          var valid = validate(doc);
          if (!valid) console.log(ajv.errorsText(validate.errors));
        }
        $scope.seriesAttributes = angular.extend({}, doc.seriesAttributes);
      }

      function loadAndValidateDefaultSegmentAttributes() {
        var doc = {
          "segmentAttributes": [[getDefaultSegmentAttributes()]]
        };
        if (schemaLoaded) {
          validate = ajv.compile({$ref: segSchemaID});
          var valid = validate(doc);
          if (!valid) console.log(ajv.errorsText(validate.errors));
        }
        return doc.segmentAttributes[0][0];
      }

      function loadSchema(uri, callback) {
        $http({
          method: 'GET',
          url: uri
        }).then(function successCallback(body) {
          callback(null, body);
        }, function errorCallback(response) {
          callback(response || new Error('Loading error: ' + response.statusCode));
        });
      }

      $scope.resetForm = function() {
        $scope.validJSON = false;
        currentLabelID = 1;
        loadDefaultSeriesAttributes();
        $scope.segments = [loadAndValidateDefaultSegmentAttributes()];
        $scope.segments[0].recommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.output = "";
      };

      $scope.onOutputChanged = function() {
        if ($scope.output.length > 0) {
          try {
            var parsedJSON = JSON.parse($scope.output);
            if (!schemaLoaded) {
              showToast("Schema for validation was not loaded.");
              return;
            }
            var valid = validate(parsedJSON);
            $scope.output = JSON.stringify(parsedJSON, null, 2);
            if (valid) {
              hideToast();
              $scope.validJSON = true;
            } else {
              $scope.validJSON = false;
              showToast(ajv.errorsText(validate.errors));
            }
          } catch(ex) {
            $scope.validJSON = false;
            showToast(ex.message);
          }
        }
      };

      function showToast(content) {
        $mdToast.show(
          $mdToast.simple()
            .content(content)
            .action('OK')
            .position('bottom right')
            .hideDelay(100000)
        );
      }

      function hideToast() {
        $mdToast.hide();
      }

      var colorPickerDefaultOptions = {
        clickOutsideToClose: true,
        openOnInput: false,
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

      // TODO: populate that from schema?
      $scope.segmentAlgorithmTypes = [
        "MANUAL",
        "SEMIAUTOMATIC",
        "AUTOMATIC"
      ];

      $scope.isSegmentAlgorithmNameRequired = function(algorithmType) {
        return ["SEMIAUTOMATIC", "AUTOMATIC"].indexOf(algorithmType) > -1;
      };

      function getDefaultSegmentAttributes() {
        return {
          LabelID: currentLabelID,
          SegmentDescription: "",
          AnatomicRegion: null,
          AnatomicRegionModifier: null,
          SegmentedPropertyTypeModifier: null
        };
      }

      $scope.addSegment = function() {
        currentLabelID += 1;
        var segment = loadAndValidateDefaultSegmentAttributes();
        segment.recommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.segments.push(segment);
        $scope.selectedIndex = $scope.segments.length-1;
      };

      $scope.removeSegment = function() {
        $scope.segments.splice($scope.selectedIndex, 1);
        if ($scope.selectedIndex-1 < 0)
          $scope.selectedIndex = 0;
        else
          $scope.selectedIndex -= 1;
        $scope.output = "";
      };

      $scope.previousSegment = function() {
        $scope.selectedIndex -= 1;
      };

      $scope.nextSegment = function() {
        $scope.selectedIndex += 1;
      };

      self.showErrors = function() {
        $scope.output = "";
        var firstError = $scope.jsonForm.$error.required[0];
        var elements = firstError.$name.split("_");
        var message = "[MISSING]: " + elements[0];
        if (elements[1] != undefined)
          message += " for segment with label id " + elements[1];
        showToast(message);
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
          "ContentCreatorName": $scope.seriesAttributes.ContentCreatorName,
          "ClinicalTrialSeriesID" : $scope.seriesAttributes.ClinicalTrialSeriesID,
          "ClinicalTrialTimePointID" : $scope.seriesAttributes.ClinicalTrialTimePointID,
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
          if (value.SegmentAlgorithmType.length > 0)
            attributes["SegmentAlgorithmType"] = value.SegmentAlgorithmType;
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
          if (value.recommendedDisplayRGBValue.color)
            attributes["recommendedDisplayRGBValue"] = self.rgbToArray(value.recommendedDisplayRGBValue.color);
          segmentAttributes.push(attributes);
        });

        var doc = {
          "seriesAttributes": seriesAttributes,
          "segmentAttributes": [segmentAttributes]
        };

        $scope.output = JSON.stringify(doc, null, 2);
        $scope.onOutputChanged();
      };

      self.rgbToArray = function(str) {
        var rgb = str.replace("rgb(", "").replace(")", "").split(", ");
        return [parseInt(rgb[0]), parseInt(rgb[1]), parseInt(rgb[2])];
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
        $scope.segment.recommendedDisplayRGBValue.color = "";
        $scope.segment.hasRecommendedColor = false;
      }
      else if (self.selectedItem.object.recommendedDisplayRGBValue != undefined) {
        $scope.segment.hasRecommendedColor = true;
        var rgb = self.selectedItem.object.recommendedDisplayRGBValue;
        $scope.segment.recommendedDisplayRGBValue.color = 'rgb('+rgb[0]+', '+rgb[1]+', '+rgb[2]+')';
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

  app.directive('resize', function ($window) {
    return {
      link: function postLink(scope, elem, attrs) {

        scope.onResizeFunction = function(element) {
          var toolbar = document.getElementById('toolbar');
          element.windowHeight = $window.innerHeight - toolbar.clientHeight;
          var newHeight = element.windowHeight-$(toolbar).height()/2;
          $(element).height(newHeight);
        };

        scope.onResizeFunction(elem);

        angular.element($window).bind('resize', function() {
          scope.onResizeFunction(elem);
          scope.$apply();
        });
      }
    };
  });

});
