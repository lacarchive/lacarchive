/**
 * @file Plugin for inserting multiple images and file links using Drupella File Manager
 */
(function($, window) {

/**
 * Global container for overridable plugin methods.
 */
var plugin = window.ckeDfm = $.extend({

/**
 * File handler for Dfm.
 */
fileHandler: function (file, win) {
  var i, selected, File, imgInsert, items, dfm = win.dfm, editor = CKEDITOR.instances[dfm.urlParam('ckeName')];
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
        items.push(plugin.fileHtml(File, !items.length && plugin.selectedHtml(editor), editor, dfm));
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
fileHtml: function(File, text) {
  return '<a href="' + File.getUrl() + '">' + (text || File.formatName()) + '</a>';
},

/**
 * Inserts given items into editor.
 */
insertItems: function(items, editor) {
  editor.insertHtml(items.join('<br />'));
},

/**
 * Plugin command helper.
 */
cmdExec: function(editor, type) {
  var width = Math.max(500, Math.min(960, parseInt(screen.availWidth * 0.75))),
  height = Math.max(300, Math.min(720, parseInt(screen.availHeight * 0.75))),
  url = window.Drupal && (Drupal.settings.dfmUrl || Drupal.settings.basePath + 'index.php?q=dfm') || '/dfm';
  url += (url.indexOf('?') == -1 ? '?' : '&') + 'fileHandler=ckeDfm.fileHandler&cmdType=' + type + '&ckeName=' + encodeURIComponent(editor.name);
  editor.popup(url, width, height);
},

/**
 * Returns selected html from ckeditor.
 */
selectedHtml: function (editor) {
  var html = '';
  try {
    var range = editor.getSelection().getRanges()[0];
    var div = editor.document.createElement('div');
    div.append(range.cloneContents());
    html = div.getHtml();
  } catch(err) {}
  return html;
},

/**
 * Plugin init callback.
 */
init: function(editor) {
  // Define commands
  editor.addCommand('DfmImageCmd', {exec : function(editor) {plugin.cmdExec(editor, 'image')}});
  editor.addCommand('DfmFileCmd', {exec : function(editor) {plugin.cmdExec(editor, 'file')}});
  // Define buttons
  editor.ui.addButton('DfmImage', {
    label: 'Insert images',
    command: 'DfmImageCmd',
    icon: this.path + 'image.png'
  });
  editor.ui.addButton('DfmFile', {
    label: 'Insert files',
    command: 'DfmFileCmd',
    icon: this.path + 'file.png'
  });
}

}, window.ckeDfm);

/**
 * CKEditor plugin definition.
 */
if (window.CKEDITOR) {
  CKEDITOR.plugins.add('dfmButtons', {init: plugin.init});
}

})(jQuery, window);
