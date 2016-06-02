(function(angular) {

  var anatomicRegionXMLPath = '../app/assets/AnatomicRegionAndModifier.xml';
  var segmentationCodesXMLPath = '../app/assets/SegmentationCategoryTypeModifier.xml';

  var app = angular.module('JSONSemanticsCreator', ['ngMaterial', 'ngMessages', 'xml'])
    .config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });

  app.controller('JSONSemanticsCreatorController', ['$scope',
    function($scope) {

      $scope.seriesAttributes = {
        ReaderID : "Reader1",
        SessionID : "Session1",
        TimePointID : "1",
        SeriesDescription : "Segmentation",
        SeriesNumber : "300",
        InstanceNumber : "1",
        BodyPartExamined :  ""
      };

      $scope.segmentAttributes = {
        LabelID: "1",
        SegmentDescription: "" // not required
        // SegmentedPropertyCategoryCode:
      };
  }]);


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
        return function filterFn(state) {
          return (state.value.indexOf(lowercaseQuery) === 0);
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
            display: code._codeMeaning,
            object: code
          };
        })
      }
    }
  );


  app.controller('AnatomicRegionController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.selectionChangedEvent = "AnatomicRegionSelectionChanged";

    $http.get(anatomicRegionXMLPath).success(function (data) {
      $scope.anatomicCodes = data.AnatomicCodes.AnatomicRegion;
      self.mappedCodes = self.codesList2codeMeaning($scope.anatomicCodes);
    });
  });


  app.controller('AnatomicRegionModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.isDisabled = true;

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
    self.selectionChangedEvent = "SegmentedPropertyCategorySelectionChanged";

    $http.get(segmentationCodesXMLPath).success(function (data) {
      $scope.segmentationCodes = data.SegmentationCodes.Category;
      self.mappedCodes = self.codesList2codeMeaning($scope.segmentationCodes);
    });
  });


  app.controller('SegmentedPropertyTypeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
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
    self.isDisabled = true;

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

})(window.angular);