// dependencies
var gulp = require('gulp');
var git = require('gulp-git');
var bump = require('gulp-bump');
var filter = require('gulp-filter');
var tag_version = require('gulp-tag-version');
var runSequence = require('run-sequence');
var wrap = require("gulp-wrap");
var gutil = require('gulp-util');

var port = 8083;

gulp.task('bump-version', function () {
	return gulp.src(['./bower.json', './package.json'])
		.pipe(bump({type: "patch"}).on('error', gutil.log))
		.pipe(gulp.dest('./'));
});

gulp.task('commit-changes', function () {
	return gulp.src('.')
		.pipe(git.commit('Bumped version number', {args: '-a'}));
});

gulp.task('tag-version', function() {
	return gulp.src('package.json')
		.pipe(tag_version());
});

gulp.task('push-changes', function (cb) {
	git.push('origin', 'master', cb);
});

gulp.task('release', function (callback) {
	runSequence(
		'bump-version',
		'build',
		'commit-changes',
		'tag-version',
		function (error) {
			if (error) {
				console.log(error.message);
			} else {
				console.log('RELEASE FINISHED SUCCESSFULLY');
			}
			callback(error);
		});
});

gulp.task('tag-version', function() {
	return gulp.src('./package.json')
		.pipe(tag_version());
});

gulp.task('build', function() {
	return gulp.src("src/angular-download.js")
		.pipe(wrap({ src: './build.txt' }, { info: require('./package.json') }))
		.pipe(gulp.dest('.'));
});

gulp.task('test', function() {
	// Be sure to return the stream
	return gulp.src(testFiles)
		.pipe(karma({
			configFile: 'karma.conf.js',
			action: 'run'
		}))
		.on('error', function(err) {
			// Make sure failed tests cause gulp to exit non-zero
			throw err;
		});
});

gulp.task('watch', function() {
	gulp.src(testFiles)
		.pipe(karma({
			configFile: 'karma.conf.js',
			action: 'watch'
		}));
});