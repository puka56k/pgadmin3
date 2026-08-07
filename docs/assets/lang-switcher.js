// pgAdmin III language switcher. Each language lives in its own folder
// (/en/, /cs/, /de/, ...); switching languages navigates to the sibling
// folder rather than toggling hidden content on a single page.
var PGADMIN3_LANGUAGES = ['ca', 'cs', 'de', 'en', 'es', 'fr', 'ja', 'lv', 'pl', 'ru', 'sr', 'zh'];

(function () {
  var STORAGE_KEY = 'pgadmin3_lang';

  function targetUrlFor(code) {
    var path = window.location.pathname;
    var currentLang = document.documentElement.lang || 'en';
    var regex = new RegExp('/' + currentLang + '(/|$)');
    var url = path.replace(regex, '/' + code + '/');
    return url === path ? null : url;
  }

  function detect() {
    var userLangs = navigator.languages || [navigator.language || navigator.userLanguage];
    for (var i = 0; i < userLangs.length; i++) {
      if (!userLangs[i]) continue;
      var code = userLangs[i].substring(0, 2).toLowerCase();
      if (PGADMIN3_LANGUAGES.indexOf(code) !== -1) return code;
    }
    return 'en';
  }

  document.addEventListener('DOMContentLoaded', function () {
    var dropdown = document.querySelector('[data-dropdown="lang"]');
    if (!dropdown) return;

    var currentLang = document.documentElement.lang || 'en';
    var stored;
    try { stored = localStorage.getItem(STORAGE_KEY); } catch (e) { /* ignore */ }
    var isAuto = !stored;

    dropdown.querySelectorAll('.dropdown-option').forEach(function (option) {
      var code = option.getAttribute('data-lang-value');
      option.classList.toggle('active', code === 'auto' ? isAuto : (code === currentLang && !isAuto));

      option.addEventListener('click', function () {
        if (code === 'auto') {
          try { localStorage.removeItem(STORAGE_KEY); } catch (e) { /* ignore */ }
          var target = targetUrlFor(detect());
          if (target) window.location.href = target;
          return;
        }
        if (code === currentLang) {
          window.pgadmin3CloseDropdowns && window.pgadmin3CloseDropdowns();
          return;
        }
        try { localStorage.setItem(STORAGE_KEY, code); } catch (e) { /* ignore */ }
        var target = targetUrlFor(code);
        if (target) window.location.href = target;
      });
    });
  });
})();
