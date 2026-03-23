/* Rewrite the home icon link to the current language root.
 *
 * The readthedocs theme renders home links with relative hrefs (e.g. "../..")
 * rather than absolute "/", so we can't select them by href value.
 * Instead we select by the classes the theme always puts on home anchors:
 *   - .icon.icon-home  (sidebar logo and breadcrumb home icon)
 *
 * Supported languages: en, de, fr, it
 */
(function () {
  var LANGS = ["en", "de", "fr", "it"];

  function detectLang() {
    var parts = window.location.pathname.replace(/^\//, "").split("/");
    if (parts.length > 0 && LANGS.indexOf(parts[0]) !== -1) {
      return parts[0];
    }
    return null;
  }

  document.addEventListener("DOMContentLoaded", function () {
    var lang = detectLang();
    if (!lang) {
      return;
    }
    var homeUrl = "/" + lang + "/";
    document.querySelectorAll("a.icon-home").forEach(function (a) {
      a.href = homeUrl;
    });
  });
})();