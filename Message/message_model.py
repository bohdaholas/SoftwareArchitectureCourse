class SingletonCounter:
    _instance = None
    _count = 0

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
        return cls._instance

    def increment(self):
        self._count += 1

    def decrement(self):
        self._count -= 1

    def get_count(self):
        return self._count
