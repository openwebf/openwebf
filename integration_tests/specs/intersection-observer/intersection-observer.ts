describe('IntersectionObserver', () => {
  let container: HTMLElement;
  let observed: HTMLElement;

  beforeEach(() => {
    container = document.createElement('div');
    document.body.appendChild(container);

    const spacer = document.createElement('div');
    spacer.style.height = '100px';
    container.appendChild(spacer);

    observed = document.createElement('div');
    observed.id = 'observed';
    observed.style.width = '200px';
    observed.style.height = '200px';
    observed.style.backgroundColor = 'red';
    container.appendChild(observed);
  });

  afterEach(() => {
    document.body.removeChild(container);
  });

  it('should trigger callback when element intersects', (done) => {
    const intersectionCallback = (entries: IntersectionObserverEntry[]) => {
      entries.forEach(entry => {
        expect(entry.target).toBe(observed);
        done();
      });
    };

    const intersectionOptions = {
      root: null,
      rootMargin: "0px",
      threshold: [0, 1]
    };

    const observer = new IntersectionObserver(intersectionCallback, intersectionOptions);
    observer.observe(observed);
  });
});
