/**
 * @file Plugin for inserting multiple images and file links using Drupella File Manager
 */
(function($, window) {

/**
 * Global container for overridable plugin methods.
 */
var plugin = window.mceDfm = $.extend({

/**
 * File handler for DFM.
 */
fileHandler: function (file, win) {
  var i, selected, File, imgInsert, items, dfm = win.dfm, editor = tinymce.editors[dfm.urlParam('mceId')];
  if (editor) {
    imgInsert = dfm.urlParam('cmdType') === 'image';
    selected = dfm.getSelectedItems();
    items = [];
    for (i = 0; File = selected[i]; i++) {
      // Inserting images
      if (imgInsert) {
        if (File.isImageSource()) {
          items.push(plugin.imageHtml(File, editor, dfm));
        }
      }
      // Inserting file links
      else if (File.isfile) {
        items.push(plugin.fileHtml(File, editor, dfm));
      }
    }
    // Insert generated content
    if (items.length) {
      plugin.insertItems(items, editor, dfm);
    }
  }
  win.close();
},

/**
 * Returns image html for the given file
 */
imageHtml: function(File) {
  return '<img src="' + File.getUrl() + '"' + (File.width ? ' width="' + File.width + '"' : '') + (File.height ? ' height="' + File.height + '"' : '') + ' alt="' + File.formatName() + '" />';
},

/**
 * Returns file link html for the given file
 */
fileHtml: function(File) {
  return '<a href="' + File.getUrl() + '">' + File.formatName() + '</a>';
},

/**
 * Inserts given items into editor.
 */
insertItems: function(items, editor) {
  editor.execCommand('mceInsertContent', false, items.join('<br />'));
},

/**
 * Plugin command helper.
 */
cmdExec: function(editor, type) {
  var width = Math.max(500, Math.min(960, parseInt(screen.availWidth * 0.75))),
  height = Math.max(300, Math.min(720, parseInt(screen.availHeight * 0.75))),
  url = window.Drupal && (Drupal.settings.dfmUrl || Drupal.settings.basePath + 'index.php?q=dfm') || '/dfm';
  url += (url.indexOf('?') == -1 ? '?' : '&') + 'fileHandler=mceDfm.fileHandler&cmdType=' + type + '&mceId=' + encodeURIComponent(editor.id);
  editor.windowManager.open({url: url, width: width, height: height});
},

/**
 * Plugin init callback.
 */
init: function(editor, pluginUrl) {
  // Define commands
  editor.addCommand('DfmImageCmd', function() {plugin.cmdExec(this, 'image')});
  editor.addCommand('DfmFileCmd', function() {plugin.cmdExec(this, 'file')});
  // Define buttons
  editor.addButton('DfmImage', {title: 'Insert images', cmd: 'DfmImageCmd', image: pluginUrl + '/image.png'});
  editor.addButton('DfmFile', {title: 'Insert files', cmd: 'DfmFileCmd', image: pluginUrl + '/file.png'});
},

/**
 * Plugin info.
 */
getInfo: function() {
	return {
		longname : 'Drupella FM: Buttons for inserting multiple images and file links',
		author : 'Drupella Web Solutions',
		authorurl : 'http://drupella.com',
		infourl : 'http://drupella.com',
		version : '7'
	};
}

}, window.mceDfm);

/**
 * Tinymce plugin definition.
 */
if (window.tinymce) {
  // Create class
  tinymce.create('tinymce.plugins.dfmButtonsPlugin', {init: plugin.init, getInfo: plugin.getInfo});
  // Add the class as a plugin
  tinymce.PluginManager.add('dfmButtons', tinymce.plugins.dfmButtonsPlugin);
}

})(jQuery, window);
