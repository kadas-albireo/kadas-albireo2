/* Language-aware home link + open external links in new tab
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
    // ── Home icon ────────────────────────────────────────────────────────────
    // Zensical (Material theme): home anchors carry the "md-logo" class,
    // both in the header and in the sidebar.
    var lang = detectLang();
    if (lang) {
      document.querySelectorAll("a.md-logo").forEach(function (a) {
        a.href = "/" + lang + "/";
      });
    }

    // ── External links → new tab ─────────────────────────────────────────────
    // Any anchor whose href points to a different origin gets
    // target="_blank" + rel="noopener noreferrer".
    document.querySelectorAll("a[href]").forEach(function (a) {
      try {
        var url = new URL(a.href, window.location.href);
        if (url.origin !== window.location.origin) {
          a.setAttribute("target", "_blank");
          a.setAttribute("rel", "noopener noreferrer");
        }
      } catch (e) {
        // ignore unparseable hrefs
      }
    });
  });
})();