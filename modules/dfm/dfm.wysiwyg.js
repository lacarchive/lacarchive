(function ($, Drupal) {
"use strict";

/**
 * Drupal behavior.
 */
Drupal.behaviors.dfmWysiwyg = {attach: function(context, set) {
  var i, id, $T, $widget, ids = set.dfmTextareas, url = set.dfmUrl;
  // Handle textarea integration.
  if (ids && url) for (i in ids) {
    if (($T = $('#' + ids[i], context).not('.dtw-processed'))[0] && $T[0].style.display !== 'none') {
      $widget = $('<div class="dfm-textarea-widget description"><a class="ins-image" href="#">' + Drupal.t('Insert image') + '</a>. <a class="ins-link" href="#">' + Drupal.t('Insert file link') + '</a>.</div>');
      $widget.find('a').bind('click', {tid: ids[i]}, dW.eTextareaWidgetClick);
      $T.addClass('dtw-processed').parent().append($widget);
    }
  }
}};

/**
 * Global container for wysiwyg utilities.
 */
var dW = window.dfmWysiwyg = {

/**
 * Returns image html for the given file
 */
imageHtml: function(File) {
  return '<img src="' + File.getUrl() + '"' + (File.width ? ' width="' + File.width + '"' : '') + (File.height ? ' height="' + File.height + '"' : '') + ' alt="' + File.formatName() + '" />';
},

/**
 * Returns file link html for the given file
 */
fileHtml: function(File, inner) {
  return '<a href="' + File.getUrl() + '">' + (inner || File.formatName() + ' (' + File.formatSize() + ')') + '</a>';
},

/**
 * Returns image or file html for the given file
 */
typeHtml: function(File, imgInsert, inner) {
  if (imgInsert && File.isImageSource()) return dW.imageHtml(File);
  if (File.isfile) return dW.fileHtml(File, inner);
},

/**
 * Gets textarea selection
 */
getSel: function(T) {
  return T.selectionStart !== undefined ? T.value.substring(T.selectionStart, T.selectionEnd) : (document.selection ? document.selection.createRange().text : '');
},

/**
 * Sets textarea selection
 */
setSel: function(T, str) {
  T.selectionStart !== undefined ? (T.value = T.value.substring(0, T.selectionStart) + str + T.value.substring(T.selectionEnd, T.value.length)) : (document.selection ? (document.selection.createRange().text = str) : (T.value += str));
},

/**
 * Sets textarea selection by joining multiple items usually of the same type.
 */
setSelMultiple: function(T, items) {
  dW.setSel(T, items.join('\n'));
},

/**
 * Insert selected files from DFM to the given textarea.
 */
insertSelected: function(dfm, T, isImg) {
  var i, File, items, html, selected = dfm.getSelectedItems();
  // Need a focusable textarea
  try {T.focus()}catch(e){return};
  // Single item
  if (selected.length == 1) {
    if (html = dW.typeHtml(selected[0], isImg, dW.getSel(T))) {
      dW.setSel(T, html);
    }
  }
  // Multiple items
  else {
    items = [];
    for (i = 0; File = selected[i]; i++) {
      if (html = dW.typeHtml(File, isImg)) {
        items.push(html);
      }
    }
    if (items.length) {
      dW.setSelMultiple(T, items, isImg);
    }
  }
},

/**
 * Returns DFM url if set.
 */
url: function() {
  return Drupal.settings.dfmUrl;
},

/**
 * Returns the DFM url with handler parameter attached.
 */
handlerUrl: function(handlerStr) {
  var url = dW.url() || '/dfm';
  return url + (url.indexOf('?') == -1 ? '?' : '&') + 'fileHandler=' + handlerStr;
},

/**
 * DFM file handler for textarea widget.
 */
textareaHandler: function(File, win) {
  var dfm = win.dfm, type = dfm.urlParam('insType'), isImg = type && type.indexOf('image') != -1;
  dW.insertSelected(dfm, $('#' + dfm.urlParam('textareaId'))[0], isImg);
  win.close();
},

/**
 * DFM file handler for BUEditor.
 */
bueHandler: function(File, win) {
  var i, input, field, fillup, els, fieldId, E, dfm = win.dfm;
  // Direct file insertion
  if (E = window.BUE.instances[dfm.urlParam('bueIndex')]) {
    dW.insertSelected(dfm, E.textArea, dfm.urlParam('insType') === 'image');
  }
  // Url field in image/link dialog
  else if (field = dW.bueFields[fieldId = dfm.urlParam('bueFid')]) {
    dW.bueFields[fieldId] = null;
    field.value = File.getUrl();
    fillup = {alt: File.formatName(), width: File.width, height: File.height};
    els = field.form.elements;
    for (i in fillup) {
      if (fillup[i] && (input = els['attr_' + i])) input.value = fillup[i];
    }
    field.focus();
  }
  win.close();
},

/**
 * DFM file handler for CKeditor image/link dialogs.
 */
ckeHandler: function(File, win) {
  window.CKEDITOR.tools.callFunction(win.dfm.urlParam('CKEditorFuncNum'), File.getUrl());
  win.close();
},

/**
 * Opens DFM for an url field of tinymce.
 */
mceBrowser: function(fieldId, url, type, win) {
  var dfmUrl = dW.url();
  if (dfmUrl && win && win.open) {
    dfmUrl += dfmUrl.indexOf('?') == -1 ? '?' : '&';
    dfmUrl += 'urlFieldId=' + encodeURIComponent(fieldId);
    dW.winOpen(dfmUrl, win);
  }
},

/**
 * Keeps track of fields that bueBrowser is opened for.
 */
bueFields: [],

/**
 * Opens DFM for an url field or for a bueditor instance.
 */
bueBrowser: function(obj, type) {
  var url = dW.handlerUrl('dfmWysiwyg.bueHandler');
  // Direct image/file insertion
  if (obj.constructor === window.BUE.instance) {
    url += '&insType=' + type + '&bueIndex=' + obj.index;
  }
  // File insertion into url field
  else if ('value' in obj) {
    url += '&bueFid=' + dW.bueFields.length;
    dW.bueFields.push(obj);
  }
  dW.winOpen(url);
},

/**
 * Browse button for bueditor.
 */
bueButton: function(fname, btnText) {
  return dW.url() ? '<input type="button" id="bue-dfm-button" name="bue_dfm_button" class="form-submit" value="'+ (btnText || Drupal.t('Browse')) +'" onclick="dfmWysiwyg.bueBrowser(this.form.elements[\''+ fname +'\'])">' : '';
},

/**
 * Click handler for textarea widget links.
 */
eTextareaWidgetClick: function(e) {
  var url = dW.handlerUrl('dfmWysiwyg.textareaHandler');
  url += '&insType=' + encodeURIComponent(this.className);
  url += '&textareaId=' + encodeURIComponent(e.data.tid);
  dW.winOpen(url);
  return false;
},

/**
 * Opens DFM window.
 */
winOpen: function(url, win) {
  var scrW = screen.availWidth, scrH = screen.availHeight,
  W = Math.max(500, Math.min(960, parseInt(scrW * 0.75))),
  H = Math.max(300, Math.min(720, parseInt(scrH * 0.75))),
  L = parseInt((scrW - W) / 2),
  T = parseInt((scrH - H) * 3 / 7);
  return (win || window).open(url, '', 'width=' + W + ',height=' + H + ',left=' + L + ',top=' + T + ',resizable=1');
}

};

/**
 * Global equivalent of DFM file handler for CKEditor.
 */
window.dfmCKHandler = dW.ckeHandler;

/**
 * Global equivalent of Bueditor browse button.
 */
window.dfmBUEButton = dW.bueButton;

})(window.jQuery, window.Drupal);
