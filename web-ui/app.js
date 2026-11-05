(() => {
  const rail = document.getElementById("card-rail");
  const toast = document.getElementById("toast");
  const navItems = [...document.querySelectorAll(".nav-item[data-page]")];
  const searchDialog = document.getElementById("search-dialog");
  const detailsDialog = document.getElementById("details-dialog");
  let activePage = "Inicio";
  let items = [];
  let selectedIndex = 0;
  let toastTimer;

  const notify = (message) => {
    toast.textContent = message;
    toast.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toast.classList.remove("show"), 2300);
  };

  const send = (type, payload = {}) => {
    const native = {
      navigate: () => window.xuperNavigate?.(payload.page || activePage),
      "play-selected": () => window.xuperPlay?.(),
      favorite: () => window.xuperFavorite?.(),
      "search-query": () => window.xuperSearch?.(payload.query || ""),
      "select-item": () => window.xuperSelectItem?.(Number(payload.index || 0)),
      back: () => window.xuperBack?.(),
      "show-all": () => window.xuperNavigate?.(activePage)
    };
    if (native[type] && (window.xuperNavigate || window.xuperPlay || window.xuperSearch)) {
      native[type]();
    } else if (window.chrome?.webview) {
      window.chrome.webview.postMessage({ type, ...payload });
    } else {
      notify("No se pudo conectar con el motor de Xuper TV.");
    }
  };

  const setPage = (page, notifyNative = true) => {
    activePage = page;
    navItems.forEach((item) => {
      const selected = item.dataset.page === page;
      item.classList.toggle("active", selected);
      item.setAttribute("aria-current", selected ? "page" : "false");
    });
    document.getElementById("shelf-title").textContent =
      page === "Inicio" ? "Continúa explorando" : page;
    if (notifyNative) send("navigate", { page });
  };

  const setStatus = (message) => {
    document.getElementById("service-status").textContent = message || "Listo";
  };

  navItems.forEach((item) => item.addEventListener("click", () => setPage(item.dataset.page)));

  const openSearch = () => {
    searchDialog.showModal();
    document.getElementById("search-query").focus();
  };
  document.getElementById("search").addEventListener("click", openSearch);
  document.getElementById("search-form").addEventListener("submit", (event) => {
    event.preventDefault();
    const query = document.getElementById("search-query").value.trim();
    if (!query) return;
    searchDialog.close();
    send("search-query", { query });
  });

  document.getElementById("watch").addEventListener("click", () => send("play-selected"));
  document.getElementById("favorite").addEventListener("click", () => send("favorite"));
  document.getElementById("more").addEventListener("click", () => {
    const item = items[selectedIndex];
    if (!item) return notify("Selecciona un título del catálogo.");
    document.getElementById("details-title").textContent = item.title || "Sin título";
    document.getElementById("details-description").textContent =
      item.description || "Contenido disponible en tu catálogo de Xuper TV.";
    detailsDialog.showModal();
  });
  document.getElementById("details-watch").addEventListener("click", () => {
    detailsDialog.close();
    send("play-selected");
  });
  document.getElementById("see-all").addEventListener("click", () => send("show-all"));

  document.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") {
      event.preventDefault();
      openSearch();
    }
    if (event.key === "Escape" && !searchDialog.open && !detailsDialog.open) send("back");
  });

  const renderCatalog = (catalogItems = [], selected = 0) => {
    items = Array.isArray(catalogItems) ? catalogItems : [];
    selectedIndex = Math.max(0, Math.min(Number(selected) || 0, Math.max(0, items.length - 1)));
    rail.replaceChildren();
    if (!items.length) {
      const empty = document.createElement("div");
      empty.className = "empty-state";
      empty.innerHTML = '<div class="empty-symbol"><svg viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="m10 9 5 3-5 3z"/></svg></div><div><strong>Sin contenido para mostrar</strong><span>El catálogo se actualizará cuando el servicio responda.</span></div>';
      rail.append(empty);
      document.getElementById("hero-title").innerHTML = "Tu próxima<br><span>gran historia.</span>";
      document.getElementById("hero-description").textContent = "Explora tu catálogo y encuentra algo que te encante.";
      document.querySelector(".hero-backdrop").style.backgroundImage = "";
      return;
    }

    const current = items[selectedIndex] || items[0];
    document.getElementById("hero-title").textContent = current.title || "Xuper TV";
    document.getElementById("hero-description").textContent =
      current.description || "Selecciona para comenzar a disfrutar.";
    const backdrop = document.querySelector(".hero-backdrop");
    if (current.artwork) { const preload = new Image(); preload.decoding = "async"; preload.fetchPriority = "high"; preload.src = current.artwork; }
    backdrop.style.backgroundImage = current.artwork
      ? 'linear-gradient(90deg,rgba(9,15,24,.9),rgba(9,15,24,.22)),url("' + current.artwork.replace(/"/g, "%22") + '")'
      : "";

    const fragment = document.createDocumentFragment();
    items.forEach((item, index) => {
      const card = document.createElement("button");
      card.className = "catalog-card" + (index === selectedIndex ? " selected" : "");
      card.type = "button";
      card.dataset.index = String(index);
      card.setAttribute("aria-label", item.title || "Abrir contenido");
      const art = document.createElement("div");
      art.className = "catalog-art";
      if (item.artwork) {
        const img = document.createElement("img");
        img.src = item.artwork;
        img.alt = "";
        img.loading = index === selectedIndex ? "eager" : "lazy";
        img.decoding = "async";
        img.fetchPriority = index === selectedIndex ? "high" : "auto";
        img.onerror = () => { img.remove(); const fallback = document.createElement("span"); fallback.textContent = "X"; fallback.className = "catalog-fallback"; art.append(fallback); };
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
      card.addEventListener("click", () => {
        selectedIndex = index;
        send("select-item", { index });
      });
      fragment.append(card);
    });
    rail.append(fragment);
    document.querySelectorAll(".catalog-card").forEach((card, index) => {
      card.classList.toggle("selected", index === selectedIndex);
    });
  };

  if (window.chrome?.webview) {
    window.chrome.webview.addEventListener("message", (event) => {
      const message = event.data || {};
      if (message.type === "catalog") renderCatalog(message.items, message.selected);
      if (message.type === "page") setPage(message.page || "Inicio", false);
      if (message.type === "status") setStatus(message.text);
    });
  }

  window.xuperUI = Object.freeze({ renderCatalog, setPage, setStatus });
  window.xuperReady?.();
})();