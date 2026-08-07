// Generic open/close behavior shared by the language and theme dropdowns
// in the site header. Each dropdown is a `.dropdown[data-dropdown]` wrapping
// a `.dropdown-trigger` button and a `.dropdown-menu`.
(function () {
  function closeAll() {
    document.querySelectorAll('.dropdown-menu.open').forEach(function (m) {
      m.classList.remove('open');
    });
    document.querySelectorAll('.dropdown-trigger').forEach(function (t) {
      t.setAttribute('aria-expanded', 'false');
    });
  }

  window.pgadmin3CloseDropdowns = closeAll;

  document.addEventListener('DOMContentLoaded', function () {
    document.querySelectorAll('.dropdown-trigger').forEach(function (trigger) {
      trigger.addEventListener('click', function (e) {
        e.stopPropagation();
        var menu = this.nextElementSibling;
        var isOpen = menu.classList.contains('open');
        closeAll();
        if (!isOpen) {
          menu.classList.add('open');
          trigger.setAttribute('aria-expanded', 'true');
        }
      });
    });

    document.addEventListener('click', closeAll);
    document.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') closeAll();
    });
  });
})();
