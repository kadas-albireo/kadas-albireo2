/* Rewrite the home icon link to the current language root.
 *
 * Zensical (Material for MkDocs) renders the home logo as an anchor with
 * class "md-header__button md-logo" and a matching sidebar element.
 * We rewrite their href so the home button always goes to /{lang}/.
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
    // Material for MkDocs / Zensical: home anchors carry the "md-logo" class
    // (both the header logo and the sidebar logo link).
    // The readthedocs theme used "icon-home" – keep that for backwards compat.
    document.querySelectorAll("a.md-logo, a.icon-home").forEach(function (a) {
      a.href = "/" + lang + "/";
    });
  });
})();