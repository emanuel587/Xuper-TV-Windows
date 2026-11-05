(() => {
  const rail = document.getElementById("card-rail");
  const toast = document.getElementById("toast");
  const navItems = [...document.querySelectorAll(".nav-item[data-page]")];
  let activePage = "Inicio";
  let toastTimer;
  const notify = (message) => {
    toast.textContent = message;
    toast.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toast.classList.remove("show"), 2300);
  };
  const send = (type, payload = {}) => {
    // Native C++ host will receive these events through WebView2.
    if (window.chrome?.webview) window.chrome.webview.postMessage({ type, ...payload });
    else notify("La conexión con el servicio se integrará en el siguiente paso.");
  };
  const setPage = (page) => {
    activePage = page;
    navItems.forEach((item) => {
      const selected = item.dataset.page === page;
      item.classList.toggle("active", selected);
      item.setAttribute("aria-current", selected ? "page" : "false");
    });
    document.getElementById("shelf-title").textContent =
      page === "Inicio" ? "Continúa explorando" : page;
    send("navigate", { page });
  };
  navItems.forEach((item) => item.addEventListener("click", () => setPage(item.dataset.page)));
  document.getElementById("watch").addEventListener("click", () => send("play-selected"));
  document.getElementById("more").addEventListener("click", () => send("details-selected"));
  document.getElementById("see-all").addEventListener("click", () => send("show-all", { page: activePage }));
  document.getElementById("search").addEventListener("click", () => send("search"));
  document.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") {
      event.preventDefault();
      send("search");
    }
    if (event.key === "Escape") send("back");
  });

  // Native app can hydrate with actual service/catalog data using postMessage.
  const renderCatalog = (items = []) => {
    rail.replaceChildren();
    if (!items.length) {
      const empty = document.createElement("div");
      empty.className = "empty-state";
      empty.innerHTML = '<div class="empty-symbol"><svg viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="m10 9 5 3-5 3z"/></svg></div><div><strong>Sin contenido para mostrar</strong><span>Cuando el catálogo esté disponible, aparecerá aquí.</span></div>';
      rail.append(empty);
      return;
    }
    const fragment = document.createDocumentFragment();
    items.forEach((item, index) => {
      const card = document.createElement("button");
      card.className = "catalog-card";
      card.type = "button";
      card.dataset.index = String(index);
      card.setAttribute("aria-label", item.title || "Abrir contenido");
      const art = document.createElement("div");
      art.className = "catalog-art";
      if (item.artwork) {
        const img = document.createElement("img");
        img.src = item.artwork;
        img.alt = "";
        img.loading = "lazy";
        art.append(img);
      } else {
        const fallback = document.createElement("span");
        fallback.textContent = "X";
        fallback.className = "catalog-fallback";
        art.append(fallback);
      }
      const title = document.createElement("span");
      title.className = "catalog-title";
      title.textContent = item.title || "Sin título";
      card.append(art, title);
      card.addEventListener("click", () => send("select-item", { index, itemId: item.id ?? null }));
      fragment.append(card);
    });
    rail.append(fragment);
  };
  if (window.chrome?.webview) {
    window.chrome.webview.addEventListener("message", (event) => {
      const message = event.data || {};
      if (message.type === "catalog") renderCatalog(message.items);
      if (message.type === "page") setPage(message.page || "Inicio");
      if (message.type === "status") document.getElementById("service-status").textContent = message.text || "Conectado";
    });
  }
  // Expose only the safe UI update surface to the host, not arbitrary execution.
  window.xuperUI = Object.freeze({ renderCatalog, setPage });
})();
