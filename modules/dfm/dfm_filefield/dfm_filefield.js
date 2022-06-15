(function ($, Drupal) {
"use strict";

/**
 * @file
 * Integrates Dfm into file field widgets.
 */

/**
 * Global container for helper methods.
 */
var dfmFileField = window.dfmFileField = {queues: {}};

/**
 * Drupal behavior to handle dfm file field integration.
 */
Drupal.behaviors.dfmFileField = {
  attach: function (context, settings) {
    var i;
    var button;
    var $buttons = $('.dfm-filefield-button', context).not('.dff-processed').addClass('dff-processed');
    for (i = 0; button = $buttons[i]; i++) {
      dfmFileField.processButton(button);
    }
    // Make remove buttons stop the relevant queues.
    $('.dfm-queue-remover', context).mousedown(function() {
      dfmFileField.removeQueue(this.id.split('-remove-button')[0]);
    });
  }
};

/**
 * Processes a dfm file field button to create a widget.
 */
dfmFileField.processButton = function (button) {
  var el;
  var url = button.getAttribute('data-dfm-url');
  var fieldId = button.id.split('-dfm-button')[0];
  // Do not process further if there is an ongoing queue.
  if (url && fieldId && !dfmFileField.processQueue(fieldId)) {
    url += (url.indexOf('?') === -1 ? '?' : '&') + 'fileHandler=dfmFileField.sendto&fieldId=' + fieldId;
    el = $(dfmFileField.createWidget(url)).insertBefore(button.parentNode)[0];
    el.parentNode.className += ' dfm-filefield-parent';
  }
  return el;
};

/**
 * Creates a dfm file field widget with the given url.
 */
dfmFileField.createWidget = function (url) {
  var $link = $('<a class="dfm-filefield-link"><span>' + Drupal.t('Select file') + '</span></a>');
  $link.attr('href', url).click(dfmFileField.eLinkClick);
  return $('<div class="dfm-filefield-widget"></div>').append($link)[0];
};

/**
 * Click event for the browser link.
 */
dfmFileField.eLinkClick = function (e) {
  window.open(this.href, '', 'width=780,height=520,resizable=1');
  e.preventDefault();
};

/**
 * Handler for dfm sendto operation.
 */
dfmFileField.sendto = function (File, win) {
  var dfm = win.dfm;
  var items = dfm.getSelectedItems();
  var fieldId = dfm.urlParam('fieldId');
  var exts = dfmFileField.getFieldExts(fieldId);
  var paths = [];
  // Prepare paths
  for (var i = 0; i < items.length; i++) {
    // Check extensions
    if (exts && !dfm.uploadValidateFileExt(items[i], exts)) {
      return;
    }
    paths.push(items[i].getPath());
  }
  // Initiate the queue
  dfmFileField.processQueue(fieldId, paths);
  win.close();
};

/**
 * Processes the file insertion queue.
 */
dfmFileField.processQueue = function(fieldId, queue) {
  var queues = dfmFileField.queues;
  var key = dfmFileField.fieldKey(fieldId);
  queue = queue || queues[key];
  if (queue) {
    queues[key] = queue;
    var path = queue.shift();
    if (!queue.length) {
      delete queues[key];
    }
    if (path) {
      dfmFileField.submit(fieldId, path);
      return true;
    }
  }
};

/**
 * Removes insertion queue of a field
 */
dfmFileField.removeQueue = function(fieldId) {
  delete dfmFileField.queues[dfmFileField.fieldKey(fieldId)];
};

/**
 * Returns field key part without the delta.
 */
dfmFileField.fieldKey = function(fieldId) {
  var parts = fieldId.split('-');
  parts.pop();
  return parts.join('-');
};

/**
 * Returns allowed extensions for a field.
 */
dfmFileField.getFieldExts = function (fieldId) {
  var settings = Drupal.settings.file;
  var elements = settings && settings.elements;
  return elements ? elements['#' + fieldId + '-upload'] : false;
};

/**
 * Submits a field widget with selected file path.
 */
dfmFileField.submit = function (fieldId, path) {
  $('#' + fieldId + '-dfm-paths').val(path);
  $('#' + fieldId + '-dfm-button').mousedown();
};

})(jQuery, Drupal);