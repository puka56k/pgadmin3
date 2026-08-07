// pgAdmin III theme switcher — Auto / Light / Dark.
// Applied as early as possible (this file is loaded synchronously from
// <head>) so there is no flash of the wrong theme before paint. The choice
// is stored so it survives navigation between language pages.
(function () {
  var STORAGE_KEY = 'pgadmin3_theme';
  var root = document.documentElement;

  function apply(value) {
    if (value === 'light' || value === 'dark') {
      root.setAttribute('data-theme', value);
    } else {
      root.removeAttribute('data-theme');
    }
  }

  function getStored() {
    try { return localStorage.getItem(STORAGE_KEY) || 'auto'; }
    catch (e) { return 'auto'; }
  }

  function setStored(value) {
    try { localStorage.setItem(STORAGE_KEY, value); }
    catch (e) { /* private browsing / storage disabled */ }
  }

  apply(getStored());

  document.addEventListener('DOMContentLoaded', function () {
    var current = getStored();
    var dropdown = document.querySelector('[data-dropdown="theme"]');
    if (!dropdown) return;

    var options = dropdown.querySelectorAll('.dropdown-option');
    options.forEach(function (option) {
      option.classList.toggle('active', option.getAttribute('data-theme-value') === current);
      option.addEventListener('click', function () {
        var value = option.getAttribute('data-theme-value');
        setStored(value);
        apply(value);
        options.forEach(function (o) {
          o.classList.toggle('active', o === option);
        });
        window.pgadmin3CloseDropdowns && window.pgadmin3CloseDropdowns();
      });
    });
  });

  window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function () {
    if (getStored() === 'auto') apply('auto');
  });
})();
