(function ($, Drupal) {
"use strict";

Drupal.behaviors.dfmAdmin = {attach: function(context) {
  // Process demo links
  $('.dfm-demo-link', context).click(eDemoLinkClick);
}};

// Click event for demo links.
var eDemoLinkClick = function(e) {
  var scrW = screen.availWidth, scrH = screen.availHeight,
  W = Math.max(500, Math.min(960, parseInt(scrW * 0.75))),
  H = Math.max(300, Math.min(720, parseInt(scrH * 0.75))),
  L = parseInt((scrW - W) / 2),
  T = parseInt((scrH - H) * 3 / 7);
  window.open(this.href, 'dfmDemo', 'width=' + W + ',height=' + H + ',left=' + L + ',top=' + T + ',resizable=1');
  return false;
};

})(window.jQuery, window.Drupal);