(function(angular) {

  var anatomicRegionXMLPath = 'assets/AnatomicRegionAndModifier.xml';
  var segmentationCodesXMLPath = 'assets/SegmentationCategoryTypeModifier.xml';

  var app = angular.module('JSONSemanticsCreator', ['ngMaterial', 'ngMessages', 'ngMdIcons', 'xml'])
    .config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });

  app.controller('JSONSemanticsCreatorController', ['$scope', '$rootScope', '$log', '$mdDialog',
    function($scope, $rootScope, $log, $mdDialog) {

      var self = this;
      self.segmentedPropertyCategory = null;
      self.segmentedPropertyType = null;
      self.segmentedPropertyTypeModifier = null;
      self.anatomicRegion = null;
      self.anatomicRegionModifier = null;

      $scope.submitted = false;

      $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
        if (data.item) {
          $scope.segmentAttributes.segmentedPropertyCategory = data.item.object;
        }
      });

      $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
        if (data.item) {
          $scope.segmentAttributes.segmentedPropertyType = data.item.object;
        }
      });

      $rootScope.$on("SegmentedPropertyTypeModifierSelectionChanged", function(event, data) {
        if (data.item) {
          $scope.segmentAttributes.segmentedPropertyTypeModifier = data.item.object;
        }
      });

      $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
        if (data.item) {
          $scope.segmentAttributes.anatomicRegion = data.item.object;
        }
      });

      $rootScope.$on("AnatomicRegionModifierSelectionChanged", function(event, data) {
        if (data.item) {
          $scope.segmentAttributes.anatomicRegionModifier = data.item.object;
        }
      });

      $scope.submitForm = function(isValid) {
        if (isValid) {
          self.createJSONOutput();
        } else {
          $scope.output = "";
        }
      };

      $scope.seriesAttributes = {
        ReaderID : "Reader1",
        SessionID : "Session1",
        TimePointID : "1",
        SeriesDescription : "Segmentation",
        SeriesNumber : 300,
        InstanceNumber : 1,
        BodyPartExamined :  ""
      };

      $scope.segmentAttributes = {
        LabelID: 1,
        SegmentDescription: "",
        AnatomicRegion: null,
        AnatomicRegionModifier: null,
        SegmentedPropertyCategoryCode: null,
        SegmentedPropertyType: null,
        SegmentedPropertyTypeModifier: null
      };

      self.segments = [];

      self.previousSegmentExists = false;
      self.nextSegmentExists = false;

      $scope.output = "";

      $scope.createSegment = function() {
        if (!self.segmentAlreadyExists()) {
          var clone = angular.extend({}, $scope.segmentAttributes);
          self.segments.push(clone);
          $rootScope.$emit("OnSegmentAdded", {});
          $scope.segmentAttributes.LabelID = "";
          $scope.segmentAttributes.SegmentDescription = "";
        } else {
          showAlert("Segment LabelID already exists",
                    "A segment with LabelID " + $scope.segmentAttributes.LabelID + " already exists.")
        }
      };

      showAlert = function(title, message) {
        $mdDialog.show(
          $mdDialog.alert()
            .parent(angular.element(document.querySelector('#popupContainer')))
            .clickOutsideToClose(true)
            .title(title)
            .textContent(message)
            .ok('OK')
        );
      };

      self.segmentAlreadyExists = function() {
        var exists = false;
        angular.forEach(self.segments, function(value, key) {
          if(value.LabelID == $scope.segmentAttributes.LabelID) {
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
        angular.forEach(self.segments, function(value, key) {
          var attributes = {};
          attributes["LabelID"] = value.LabelID;
          if (value.SegmentDescription.length > 0)
            attributes["SegmentDescription"] = value.SegmentDescription;
          if (value.anatomicRegion)
            attributes["AnatomicRegion"] = getCodeSequenceAttributes(value.anatomicRegion);
          if (value.anatomicRegionModifier)
            attributes["AnatomicRegionModifier"] = getCodeSequenceAttributes(value.anatomicRegionModifier);
          if (value.segmentedPropertyCategory)
            attributes["SegmentedPropertyCategoryCode"] = getCodeSequenceAttributes(value.segmentedPropertyCategory);
          if (value.segmentedPropertyType)
            attributes["SegmentedPropertyType"] = getCodeSequenceAttributes(value.segmentedPropertyType);
          if (value.segmentedPropertyTypeModifier)
            attributes["SegmentedPropertyTypeModifier"] = getCodeSequenceAttributes(value.segmentedPropertyTypeModifier);
          segmentAttributes.push(attributes);
        });

        var doc = {
          "seriesAttributes": seriesAttributes,
          "segmentAttributes": segmentAttributes
        };

          $scope.output = doc;
      };
  }]);

  function getCodeSequenceAttributes(codeSequence) {
    if (codeSequence != null && codeSequence != undefined)
      return {"codeValue":codeSequence._codeValue,
              "codingSchemeDesignator":codeSequence._codingScheme,
              "codeMeaning":codeSequence._codeMeaning}
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
        $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem});
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
        list.sort(function(a,b) {return (a._codeMeaning > b._codeMeaning) ? 1 : ((b._codeMeaning > a._codeMeaning) ? -1 : 0);});
        return list.map(function (code) {
          return {
            value: code._codeMeaning.toLowerCase(),
            contextGroupName : code._contextGroupName,
            display: code._codeMeaning,
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

    $http.get(anatomicRegionXMLPath).success(function (data) {
      $scope.anatomicCodes = data.AnatomicCodes.AnatomicRegion;
      self.mappedCodes = self.codesList2codeMeaning($scope.anatomicCodes);
    });

    $rootScope.$on("OnSegmentAdded", function(event, data) {
      self.searchText = "";
    });
  });


  app.controller('AnatomicRegionModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "AnatomicRegionModifierSelectionChanged";

    $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
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

    $http.get(segmentationCodesXMLPath).success(function (data) {
      $scope.segmentationCodes = data.SegmentationCodes.Category;
      self.mappedCodes = self.codesList2codeMeaning($scope.segmentationCodes);
    });

    $rootScope.$on("OnSegmentAdded", function(event, data) {
      self.searchText = "";
    });
  });


  app.controller('SegmentedPropertyTypeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.required = true;
    self.floatingLabel = "Type";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeSelectionChanged";

    $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
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
    self.floatingLabel = "Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeModifierSelectionChanged";

    $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
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
  
})(window.angular);